/*
 * ProFTPD: mod_sql -- SQL frontend
 * Copyright (c) 1998-1999 Johnie Ingram.
 * Copyright (c) 2001 Andrew Houghton.
 * Copyright (c) 2004-2014 TJ Saunders
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, Andrew Houghton and other respective copyright
 * holders give permission to link this program with OpenSSL, and distribute
 * the resulting executable, without including the source code for OpenSSL in
 * the source distribution.
 *
 * $Id: mod_sql.c,v 1.247 2014-02-15 08:31:25 castaglia Exp $
 */

#include "conf.h"
#include "privs.h"
#include "mod_sql.h"

#define MOD_SQL_VERSION			"mod_sql/4.3"

#if defined(HAVE_CRYPT_H) && !defined(AIX4) && !defined(AIX5)
# include <crypt.h>
#endif

#if defined(HAVE_OPENSSL) || defined(PR_USE_OPENSSL)
# include <openssl/evp.h>
#endif

/* default information for tables and fields */
#define MOD_SQL_DEF_USERTABLE			"users"
#define MOD_SQL_DEF_USERNAMEFIELD		"userid"
#define MOD_SQL_DEF_USERUIDFIELD		"uid"
#define MOD_SQL_DEF_USERGIDFIELD		"gid"
#define MOD_SQL_DEF_USERPASSWORDFIELD		"passwd"
#define MOD_SQL_DEF_USERSHELLFIELD		"shell"
#define MOD_SQL_DEF_USERHOMEDIRFIELD		"homedir"

#define MOD_SQL_DEF_GROUPTABLE			"groups"
#define MOD_SQL_DEF_GROUPNAMEFIELD		"groupname"
#define MOD_SQL_DEF_GROUPGIDFIELD		"gid"
#define MOD_SQL_DEF_GROUPMEMBERSFIELD		"members"

/* default minimum ID / default UID / default GID info. 
 * UIDs and GIDs less than MOD_SQL_MIN_USER_UID and MOD_SQL_MIN_USER_GID,
 * respectively, get automatically mapped to the defaults, below.  These can
 * be overridden using directives
 */
#define MOD_SQL_MIN_USER_UID		999
#define MOD_SQL_MIN_USER_GID		999
#define MOD_SQL_DEF_UID			65533
#define MOD_SQL_DEF_GID			65533

#define MOD_SQL_BUFSIZE			32

/* Named Query defines */
#define SQL_SELECT_C		"SELECT"
#define SQL_INSERT_C		"INSERT"
#define SQL_UPDATE_C		"UPDATE"
#define SQL_FREEFORM_C		"FREEFORM"

/* SQLEngine flags */
#define SQL_ENGINE_FL_AUTH	0x001
#define SQL_ENGINE_FL_LOG	0x002

/* SQLLog flags */
#define SQL_LOG_FL_IGNORE_ERRORS	0x001

/* authmask defines */
#define SQL_AUTH_USERS             (1<<0)
#define SQL_AUTH_GROUPS            (1<<1)
#define SQL_AUTH_USERSET           (1<<4)
#define SQL_AUTH_GROUPSET          (1<<5)
#define SQL_FAST_USERSET           (1<<6)
#define SQL_FAST_GROUPSET          (1<<7)

#define SQL_GROUPS             (cmap.authmask & SQL_AUTH_GROUPS)
#define SQL_USERS              (cmap.authmask & SQL_AUTH_USERS)
#define SQL_GROUPSET           (cmap.authmask & SQL_AUTH_GROUPSET)
#define SQL_USERSET            (cmap.authmask & SQL_AUTH_USERSET)
#define SQL_FASTGROUPS         (cmap.authmask & SQL_FAST_GROUPSET)
#define SQL_FASTUSERS          (cmap.authmask & SQL_FAST_USERSET)

/*
 * externs, function signatures.. whatever necessary to make
 * the compiler happy..
 */

module sql_module;

unsigned long pr_sql_opts = 0UL;
unsigned int pr_sql_conn_policy = 0;

/* For tracking the size of deleted files. */
static off_t sql_dele_filesz = 0;

/* It is best if this value is larger than the PR_TUNABLE_BUFFER_SIZE value.
 * PR_TUNABLE_BUFFER_SIZE controls how much network data from a client at
 * a time we might read; by keeping the statement buffer size larger, we reduce
 * the chance of handling data from the network which exceeds the statement
 * buffer length.
 */
#define SQL_MAX_STMT_LEN	4096

static char *sql_prepare_where(int, cmd_rec *, int, ...);
#define SQL_PREPARE_WHERE_FL_NO_TAGS	0x00001

static modret_t *process_named_query(cmd_rec *cmd, char *name, int flags);
static const char *resolve_long_tag(cmd_rec *, char *);
static int resolve_numeric_tag(cmd_rec *, char *);
static char *resolve_short_tag(cmd_rec *, char);
static const char *get_named_conn_backend(const char *);
static char *get_query_named_conn(config_rec *c);
static void set_named_conn_backend(const char *);

MODRET cmd_getgrent(cmd_rec *);
MODRET cmd_setgrent(cmd_rec *);

MODRET sql_lookup(cmd_rec *);
static cmdtable *sql_set_backend(const char *);

static pool *sql_pool = NULL;

static const char *trace_channel = "sql";

/*
 * cache typedefs
 */

#define CACHE_SIZE         13

typedef struct cache_entry {
  struct cache_entry *list_next;
  struct cache_entry *bucket_next;
  void *data;
} cache_entry_t;

/* This struct holds invariant information for the current session */

static struct {
  /* Info valid after getpwnam(). */
  char *authuser;               /* current authorized user */
  struct passwd *authpasswd;    /* and their passwd struct */

  /* Generic status information. */
  int engine;                   /* is mod_sql on? */
  int authmask;                 /* authentication mask.
                                 * see set_sqlauthenticate for info */
  /* User table and field information. */
  char *usrtable;               /* user info table name */
  char *usrfield;               /* user name field */
  char *pwdfield;               /* user password field */
  char *uidfield;               /* user UID field */
  char *gidfield;               /* user GID field */
  char *homedirfield;           /* user homedir field */
  char *shellfield;             /* user login shell field */
  char *userwhere;              /* user where clause */

  char *usercustom;		/* custom users query (by name) */
  char *usercustombyid;		/* custom users query (by UID) */
  char *usercustomuserset;	/* custom query to get 'userset' users */
  char *usercustomusersetfast;	/* custom query to get 'usersetfast' users */

  /* Group table and field information. */
  char *grptable;               /* group info table name */
  char *grpfield;               /* group name field */
  char *grpgidfield;            /* group GID field */
  char *grpmembersfield;        /* group members field */
  char *groupwhere;             /* group where clause */

  char *groupcustombyname;	/* custom group query (by name) */
  char *groupcustombyid;	/* custom group query (by GID) */
  char *groupcustommembers;	/* custom group query (user members only) */
  char *groupcustomgroupset;	/* custom query to get 'groupset' groups */
  char *groupcustomgroupsetfast;/* custom query to get 'groupsetfast' groups */

  /* Other information. */
  array_header *auth_list;      /* auth handler list */
  char *defaulthomedir;         /* default homedir if no field specified */

  uid_t minid;                  /* users UID must be this or greater */
  uid_t minuseruid;             /* users UID must be this or greater */
  gid_t minusergid;             /* users UID must be this or greater */
  uid_t defaultuid;             /* default UID if none in database */
  gid_t defaultgid;             /* default GID if none in database */

  cache_entry_t *curr_group;    /* next group in group array for getgrent */
  cache_entry_t *curr_passwd;   /* next passwd in passwd array for getpwent */
  int group_cache_filled;
  int passwd_cache_filled;

  /* Cache negative, as well as positive, lookups */
  unsigned char negative_cache;

  /* mod_ratio data -- someday this needs to be removed from mod_sql. */
  char *sql_fstor;              /* fstor int(11) NOT NULL DEFAULT '0', */
  char *sql_fretr;              /* fretr int(11) NOT NULL DEFAULT '0', */
  char *sql_bstor;              /* bstor int(11) NOT NULL DEFAULT '0', */
  char *sql_bretr;              /* bretr int(11) NOT NULL DEFAULT '0', */

  char *sql_frate;              /* frate int(11) NOT NULL DEFAULT '5', */
  char *sql_fcred;              /* fcred int(2) NOT NULL DEFAULT '15', */
  char *sql_brate;              /* brate int(11) NOT NULL DEFAULT '5', */
  char *sql_bcred;              /* bcred int(2) NOT NULL DEFAULT '150000', */

  /* Precomputed strings. */
  char *usrfields;
  char *grpfields;
} cmap;

/* For handling the SQLBackend directive */
struct sql_backend {
  struct sql_backend *next, *prev;
  const char *backend;
  cmdtable *cmdtab;
};

static struct sql_backend *sql_backends = NULL;
static unsigned int sql_nbackends = 0;
static cmdtable *sql_cmdtable = NULL, *sql_default_cmdtable = NULL;

/* For handling the SQLNamedConnectInfo directives */
struct sql_named_conn {
  struct sql_named_conn *next, *prev;
  const char *conn_name;
  unsigned int conn_policy;
  const char *backend;
};

static struct sql_named_conn *sql_named_conns = NULL;

#define MOD_SQL_DEF_CONN_NAME		"default"

/* Look up the backend to use for the named connection. */
static const char *get_named_conn_backend(const char *conn_name) {
  if (conn_name == NULL) {
    errno = EINVAL;
    return NULL;
  }

  /* Make sure that "default" is handled as a reserved connection name. */
  if (strcmp(conn_name, MOD_SQL_DEF_CONN_NAME) == 0) {
    errno = EACCES;
    return NULL;
  }

  if (sql_named_conns != NULL) {
    struct sql_named_conn *snc;

    for (snc = sql_named_conns; snc; snc = snc->next) {
      if (strcmp(snc->conn_name, conn_name) == 0) {
        return snc->backend;
      }
    }
  }

  errno = ENOENT;
  return NULL;
}

static char *get_query_named_conn(config_rec *c) {
  char *conn_name = MOD_SQL_DEF_CONN_NAME;

  if (strcasecmp(c->argv[0], SQL_SELECT_C) == 0 ||
      strcasecmp(c->argv[0], SQL_FREEFORM_C) == 0) {
    conn_name = c->argv[2];

  } else if (strcasecmp(c->argv[0], SQL_INSERT_C) == 0 ||
             strcasecmp(c->argv[0], SQL_UPDATE_C) == 0) {
    conn_name = c->argv[3];
  }

  return conn_name;
}

static void set_named_conn_backend(const char *conn_name) {
  const char *backend;

  if (conn_name == NULL) {
    /* The caller is requesting that we request the backend to the default. */
    sql_cmdtable = sql_default_cmdtable;
    return;
  }

  if (strcmp(conn_name, MOD_SQL_DEF_CONN_NAME) == 0) {
    sql_cmdtable = sql_default_cmdtable;
    return;
  }

  backend = get_named_conn_backend(conn_name);
  if (backend == NULL) {
    sql_log(DEBUG_INFO, MOD_SQL_VERSION
      ": named connection '%s' does not exist, defaulting to '%s'",
      conn_name, MOD_SQL_DEF_CONN_NAME);
    sql_cmdtable = sql_default_cmdtable;
    return;
  }

  if (sql_set_backend(backend) == NULL) {
    sql_log(DEBUG_INFO, "unable to load SQL backend '%s': %s", backend,
      strerror(errno));

  } else {
    sql_log(DEBUG_INFO, "using named connection '%s', backend '%s' for query",
      conn_name, backend);
  }
}

/*
 * cache functions
 */

typedef unsigned int (* val_func)(const void *); 
typedef int (* cmp_func)(const void *, const void *);

typedef struct {
  /* memory pool for this object */
  pool *pool;

  /* cache buckets */
  cache_entry_t *buckets[ CACHE_SIZE ];

  /* cache functions */
  val_func hash_val;
  cmp_func cmp;

  /* list pointers */
  cache_entry_t *head;

  /* list size */
  unsigned int nelts;
} cache_t;

static cache_t *group_name_cache = NULL;
static cache_t *group_gid_cache = NULL;
static cache_t *passwd_name_cache = NULL;
static cache_t *passwd_uid_cache = NULL;

static cache_t *make_cache(pool *p, val_func hash_val, cmp_func cmp) {
  cache_t *res;

  if (p == NULL ||
      hash_val == NULL || 
      cmp == NULL)
    return NULL;

  res = (cache_t *) pcalloc(p, sizeof(cache_t));

  res->pool = p;
  res->hash_val = hash_val;
  res->cmp = cmp;

  res->head = NULL;

  res->nelts = 0;

  return res;
}

static cache_entry_t *cache_addentry(cache_t *cache, void *data) {
  cache_entry_t *entry;
  int hashval;

  if (cache == NULL ||
      data == NULL)
    return NULL;

  /* create the entry */
  entry = (cache_entry_t *) pcalloc(cache->pool, sizeof(cache_entry_t));
  entry->data = data;

  /* deal with the list */

  if (cache->head == NULL) {
    cache->head = entry;

  } else {
    entry->list_next = cache->head;
    cache->head = entry;
  }

  /* deal with the buckets */
  hashval = cache->hash_val(data) % CACHE_SIZE;
  if (cache->buckets[hashval] == NULL) {
    cache->buckets[hashval] = entry;

  } else {
    entry->bucket_next = cache->buckets[hashval];
    cache->buckets[hashval] = entry;
  }
  
  cache->nelts++;

  return entry;
}

static void *cache_findvalue(cache_t *cache, void *data) {
  cache_entry_t *entry;
  int hashval;

  if (cache == NULL ||
      data == NULL)
    return NULL;
  
  hashval = cache->hash_val(data) % CACHE_SIZE;

  entry = cache->buckets[hashval];
  while (entry != NULL) {
    pr_signals_handle();

    if (cache->cmp(data, entry->data))
      break;
    else
      entry = entry->bucket_next;
  }

  return (entry == NULL ? NULL : entry->data);
}

cmd_rec *_sql_make_cmd(pool *p, int argc, ...) {
  register unsigned int i = 0;
  pool *newpool = NULL;
  cmd_rec *cmd = NULL;
  va_list args;

  newpool = make_sub_pool(p);
  cmd = pcalloc(newpool, sizeof(cmd_rec));
  cmd->argc = argc;
  cmd->stash_index = -1;
  cmd->pool = newpool;
  
  cmd->argv = pcalloc(newpool, sizeof(void *) * (argc + 1));
  cmd->tmp_pool = newpool;
  cmd->server = main_server;

  va_start(args, argc);

  for (i = 0; i < argc; i++)
    cmd->argv[i] = (void *) va_arg(args, char *);

  va_end(args);

  cmd->argv[argc] = NULL;

  return cmd;
}

static int check_response(modret_t *mr, int flags) {
  if (!MODRET_ISERROR(mr))
    return 0;

  sql_log(DEBUG_WARN, "%s", "unrecoverable backend error");
  sql_log(DEBUG_WARN, "error: '%s'", mr->mr_numeric);
  sql_log(DEBUG_WARN, "message: '%s'", mr->mr_message);

  pr_log_pri(PR_LOG_ERR, MOD_SQL_VERSION
    ": unrecoverable backend error: (%s) %s", mr->mr_numeric, mr->mr_message);
  pr_log_pri(PR_LOG_ERR, MOD_SQL_VERSION
    ": check the SQLLogFile for more details");

  if (!(flags & SQL_LOG_FL_IGNORE_ERRORS) &&
      !(pr_sql_opts & SQL_OPT_NO_DISCONNECT_ON_ERROR)) {
    pr_session_disconnect(&sql_module, PR_SESS_DISCONNECT_BY_APPLICATION,
      "Database error");
  }

  if (flags & SQL_LOG_FL_IGNORE_ERRORS) {
    sql_log(DEBUG_FUNC, "SQLLog IGNORE_ERRORS in effect, not exiting");

  } else {
    sql_log(DEBUG_FUNC, "SQLOption noDisconnectOnError in effect, not exiting");
  }

  return -1;
}

static modret_t *_sql_dispatch(cmd_rec *cmd, char *cmdname) {
  modret_t *mr = NULL;
  register unsigned int i = 0;

  for (i = 0; sql_cmdtable[i].command; i++) {
    if (strcmp(cmdname, sql_cmdtable[i].command) == 0) {
      pr_signals_block();
      mr = sql_cmdtable[i].handler(cmd);
      pr_signals_unblock();
      return mr;
    }
  }

  sql_log(DEBUG_WARN, "unknown backend handler '%s'", cmdname);
  return PR_ERROR(cmd);
}

static struct sql_backend *sql_get_backend(const char *backend) {
  struct sql_backend *sb;

  if (!sql_backends)
    return NULL;

  for (sb = sql_backends; sb; sb = sb->next) {
    if (strcasecmp(sb->backend, backend) == 0)
      return sb;
  }

  return NULL;
}

/* This function is used by mod_sql backends, to register their
 * individual backend command tables with the main mod_sql module.
 */
int sql_register_backend(const char *backend, cmdtable *cmdtab) {
  struct sql_backend *sb;

  if (!backend || !cmdtab) {
    errno = EINVAL;
    return -1;
  }

  if (!sql_pool) {
    sql_pool = make_sub_pool(permanent_pool);
    pr_pool_tag(sql_pool, MOD_SQL_VERSION);
  }

  /* Check to see if this backend has already been registered. */
  sb = sql_get_backend(backend);
  if (sb) {
    errno = EEXIST;
    return -1;
  }

  sb = pcalloc(sql_pool, sizeof(struct sql_backend));
  sb->backend = backend;
  sb->cmdtab = cmdtab;

  if (sql_backends) {
    sql_backends->prev = sb;
    sb->next = sql_backends;
  }

  sql_backends = sb;
  sql_nbackends++;

  return 0;
}

/* Used by mod_sql backends to unregister their backend command tables
 * from the main mod_sql module.
 */
int sql_unregister_backend(const char *backend) {
  struct sql_backend *sb;

  if (!backend) {
    errno = EINVAL;
    return -1;
  }

  /* Check to see if this backend has been registered. */
  sb = sql_get_backend(backend);
  if (!sb) {
    errno = ENOENT;
    return -1;
  }

#if !defined(PR_SHARED_MODULE)
  /* If there is only one registered backend, it cannot be removed.
   */
  if (sql_nbackends == 1) {
    errno = EPERM;
    return -1;
  }

  /* Be sure to handle the case where this is the currently active backend. */
  if (sql_cmdtable &&
      sb->cmdtab == sql_cmdtable) {
    errno = EACCES;
    return -1;
  }
#endif

  /* Remove this backend from the linked list. */
  if (sb->prev) {
    sb->prev->next = sb->next;

  } else {
    /* This backend is the start of the sql_backends list (prev is NULL),
     * so we need to update the list head pointer as well.
     */
    sql_backends = sb->next;
  }

  if (sb->next)
    sb->next->prev = sb->prev;

  sb->prev = sb->next = NULL;

  sql_nbackends--;

  /* NOTE: a counter should be kept of the number of unregistrations,
   * as the memory for a registration is not freed on unregistration.
   */

  return 0;
}

/* Determine which backend to use.
 *
 * If there is only one registered backend to use, the decision is easy.
 *
 * If there are more than one backends, default to using the first
 * entry in the linked list (last backend module registered).  Check
 * for a given backend name argument, if any, to see if that backend
 * is available.
 */
static cmdtable *sql_set_backend(const char *backend) {
  if (sql_nbackends == 0 ||
      sql_backends == NULL) {
    pr_log_debug(DEBUG0, MOD_SQL_VERSION ": no SQL backends registered");
    sql_log(DEBUG_INFO, "%s", "no SQL backends registered");
    errno = ENOENT;
    return NULL;
  }

  if (sql_nbackends == 1) {
    pr_log_debug(DEBUG8, MOD_SQL_VERSION ": defaulting to '%s' backend",
      sql_backends->backend);
    sql_log(DEBUG_INFO, "defaulting to '%s' backend", sql_backends->backend);
    sql_cmdtable = sql_backends->cmdtab;

  } else if (sql_nbackends > 1) {
    if (backend) {
      struct sql_backend *b;

      for (b = sql_backends; b; b = b->next) {
        if (strcasecmp(b->backend, backend) == 0) {
          sql_log(DEBUG_INFO, "using SQLBackend '%s'", backend);
          sql_cmdtable = b->cmdtab;
          break;
        }
      }

      /* If no match is found, default to using the last entry in the list. */
      if (!sql_cmdtable) {
        b = sql_backends;
        while (b->next != NULL) {
          pr_signals_handle();
          b = b->next;
        }

        sql_log(DEBUG_INFO,
          "SQLBackend '%s' not found, defaulting to '%s' backend",
          backend, b->backend);
        sql_cmdtable = b->cmdtab;
      }

    } else {
      /* Default to using the last entry in the list. */
      struct sql_backend *b = sql_backends;

      while (b->next != NULL) {
        pr_signals_handle();
        b = b->next;
      }

      sql_log(DEBUG_INFO, "defaulting to '%s' backend",
        b->backend);
      sql_cmdtable = b->cmdtab;
    }
  }

  return sql_cmdtable;
}

/* Default SQL password handlers (a.k.a. "AuthTypes") provided by mod_sql. */

static modret_t *sql_auth_crypt(cmd_rec *cmd, const char *plaintext,
    const char *ciphertext) {
  char *res = NULL;

  if (*ciphertext == '\0') {
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  res = (char *) crypt(plaintext, ciphertext);
  if (res == NULL) {
    sql_log(DEBUG_WARN, "error using crypt(3): %s", strerror(errno));
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  if (strcmp(res, ciphertext) == 0) {
    return PR_HANDLED(cmd);
  }

  return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
}

static modret_t *sql_auth_plaintext(cmd_rec *cmd, const char *plaintext,
    const char *ciphertext) {

  if (*ciphertext == '\0') {
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  if (strcmp(plaintext, ciphertext) == 0) {
    return PR_HANDLED(cmd);
  }

  return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
}

static modret_t *sql_auth_empty(cmd_rec *cmd, const char *plaintext,
    const char *ciphertext) {

  if (strcmp(ciphertext, "") == 0) {
    return PR_HANDLED(cmd);
  }

  return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
}

static modret_t *sql_auth_backend(cmd_rec *cmd, const char *plaintext,
    const char *ciphertext) {
  modret_t *mr = NULL;

  if (*ciphertext == '\0') {
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME,
    plaintext, ciphertext), "sql_checkauth");
  return mr;
}

#if defined(HAVE_OPENSSL) || defined(PR_USE_OPENSSL)
static modret_t *sql_auth_openssl(cmd_rec *cmd, const char *plaintext,
    const char *ciphertext) {

  /* The ciphertext argument is a combined digest name and hashed value, of
   * the form "{digest}hash".
   */

  EVP_MD_CTX md_ctxt;
  EVP_ENCODE_CTX base64_ctxt;
  const EVP_MD *md;

  /* According to RATS, the output buffer (buf) for EVP_EncodeBlock() needs to
   * be 4/3 the size of the input buffer (mdval).  Let's make it easy, and
   * use an output buffer that's twice the size of the input buffer.
   */
  unsigned char buf[EVP_MAX_MD_SIZE*2], mdval[EVP_MAX_MD_SIZE];
  unsigned int mdlen;

  char *digestname;             /* ptr to name of the digest function */
  char *hashvalue;              /* ptr to hashed value we're comparing to */
  char *copytext;               /* temporary copy of the ciphertext string */

  if (ciphertext[0] != '{') {
    sql_log(DEBUG_WARN, "%s", "no digest found in password hash");
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  /* We need a copy of the ciphertext. */
  copytext = pstrdup(cmd->tmp_pool, ciphertext);

  digestname = copytext + 1;

  hashvalue = (char *) strchr(copytext, '}');
  if (hashvalue == NULL) {
    sql_log(DEBUG_WARN, "%s", "no terminating '}' for digest");
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  *hashvalue = '\0';
  hashvalue++;

  OpenSSL_add_all_digests();

  md = EVP_get_digestbyname(digestname);
  if (md == NULL) {
    sql_log(DEBUG_WARN, "no such digest '%s' supported", digestname);
    return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
  }

  EVP_DigestInit(&md_ctxt, md);
  EVP_DigestUpdate(&md_ctxt, plaintext, strlen(plaintext));
  EVP_DigestFinal(&md_ctxt, mdval, &mdlen);

  memset(buf, '\0', sizeof(buf));
  EVP_EncodeInit(&base64_ctxt);
  EVP_EncodeBlock(buf, mdval, (int) mdlen);

  if (strcmp((char *) buf, hashvalue) == 0) {
    return PR_HANDLED(cmd);
  }

  return PR_ERROR_INT(cmd, PR_AUTH_BADPWD);
}
#endif

struct sql_authtype_handler {
  struct sql_authtype_handler *next, *prev;
  pool *pool;
  const char *name;
  modret_t *(*cb)(cmd_rec *, const char *, const char *); 
};

static struct sql_authtype_handler *sql_auth_list = NULL;

static struct sql_authtype_handler *sql_get_authtype(const char *name) {
  if (sql_auth_list) {
    struct sql_authtype_handler *sah;

    for (sah = sql_auth_list; sah; sah = sah->next) {
      if (strcasecmp(sah->name, name) == 0) {
        return sah;
      }
    }
  }

  errno = ENOENT;
  return NULL;
}

int sql_register_authtype(const char *name,
    modret_t *(*cb)(cmd_rec *, const char *, const char *)) {
  struct sql_authtype_handler *sah;
  pool *p;

  if (name == NULL ||
      cb == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Check for duplicates. */
  sah = sql_get_authtype(name);
  if (sah != NULL) {
    errno = EEXIST;
    return -1;
  }

  if (sql_pool == NULL) {
    sql_pool = make_sub_pool(permanent_pool);
    pr_pool_tag(sql_pool, MOD_SQL_VERSION);
  }

  p = pr_pool_create_sz(sql_pool, 128);
  sah = pcalloc(p, sizeof(struct sql_authtype_handler));
  sah->pool = p;
  sah->name = pstrdup(sah->pool, name);
  sah->cb = cb;

  if (sql_auth_list) {
    sql_auth_list->prev = sah;
    sah->next = sql_auth_list;
  }
    
  sql_auth_list = sah;
  return 0;
}

int sql_unregister_authtype(const char *name) {

  if (name == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (sql_auth_list) {
    struct sql_authtype_handler *sah;

    for (sah = sql_auth_list; sah; sah = sah->next) {
      if (strcasecmp(sah->name, name) == 0) {
        if (sah->prev) {
          sah->prev->next = sah->next;

        } else {
          /* This backend is the start of the list, so update the list
           * head pointer as well.
           */
          sql_auth_list = sah->next;       
        }

        if (sah->next) {
          sah->next->prev = sah->prev;
        }

        destroy_pool(sah->pool);
        return 0;
      }
    }
  }

  errno = ENOENT;
  return -1;
}

/*****************************************************************
 *
 * INTERNAL HELPER FUNCTIONS
 *
 *****************************************************************/

/* find who core thinks is the user, and return a (backend-escaped) 
 * version of that name */
static char *_sql_realuser(cmd_rec *cmd) {
  modret_t *mr = NULL;
  char *user = NULL;

  /* this is the userid given by the user */
  user = pr_table_get(session.notes, "mod_auth.orig-user", NULL);

  /* Do we need to check for useralias? see mod_time.c, get_user_cmd_times(). */
  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
    user), "sql_escapestring");
  if (check_response(mr, 0) < 0) {
    return NULL;
  }

  return mr ? (char *) mr->data : NULL;
}

static int sql_define_conn(pool *p, char *conn_name, char *user, char *passwd,
    char *info, char *ttl) {
  cmd_rec *cmd = NULL;
  modret_t *mr = NULL;

  cmd = _sql_make_cmd(p, 5, conn_name, user, passwd, info, ttl);
  mr = _sql_dispatch(cmd, "sql_defineconnection");
  if (check_response(mr, 0) < 0) {
    return -1;
  }

  SQL_FREE_CMD(cmd);

  if (pr_sql_conn_policy == SQL_CONN_POLICY_PERCONN) {
    /* Open a database connection now, so that we have a database connection
     * for the lifetime of the client's connection to the server.
     */
    cmd = _sql_make_cmd(p, 1, conn_name);
    mr = _sql_dispatch(cmd, "sql_open");
    if (check_response(mr, 0) < 0) {
      return -1;
    }

    SQL_FREE_CMD(cmd);
  }

  return 0;
}

static char *sql_prepare_where(int flags, cmd_rec *cmd, int cnt, ...) {
  int i, flag, nclauses = 0;
  int curr_avail;
  char *buf = "", *res;
  va_list dummy;

  /* Allocate one byte more for the terminating NUL. */
  res = pcalloc(cmd->tmp_pool, SQL_MAX_STMT_LEN + 1);

  flag = 0;
  va_start(dummy, cnt);
  for (i = 0; i < cnt; i++) {
    char *clause = va_arg(dummy, char *);
    if (clause != NULL &&
        *clause != '\0') {
      nclauses++;

      if (flag++)
        buf = pstrcat(cmd->tmp_pool, buf, " AND ", NULL);
      buf = pstrcat(cmd->tmp_pool, buf, "(", clause, ")", NULL);
    }
  }
  va_end(dummy);

  if (nclauses == 0)
    return NULL;

  if (!(flags & SQL_PREPARE_WHERE_FL_NO_TAGS)) {
    char *curr, *tmp;

    /* Process variables in WHERE clauses, except any "%{num}" references.
     *
     * curr_avail is deliberately set to be one byte less than the allocated
     * length, to make sure that there is a terminating NUL.
     */
    curr = res;
    curr_avail = SQL_MAX_STMT_LEN;

    for (tmp = buf; *tmp; ) {
      char *str;
      modret_t *mr;
      size_t taglen;

      pr_signals_handle();

      if (*tmp == '%') {
        char *tag = NULL;

        if (*(++tmp) == '{') {
          char *query = NULL;

          if (*tmp != '\0')
            query = ++tmp;

          while (*tmp && *tmp != '}')
            tmp++;

          tag = pstrndup(cmd->tmp_pool, query, (tmp - query));
          if (tag) {
            str = (char *) resolve_long_tag(cmd, tag);
            if (str == NULL) {
              str = pstrdup(cmd->tmp_pool, "");
            }

            mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2,
              MOD_SQL_DEF_CONN_NAME, str), "sql_escapestring");
            if (check_response(mr, 0) < 0)
              return NULL;

            /* Make sure we don't write too much data. */
            taglen = strlen(mr->data);
            if (curr_avail > taglen) {
              sstrcat(curr, mr->data, curr_avail);
              curr += taglen;
              curr_avail -= taglen;

            } else {
              /* Log when this happens; it means we have more input than buffer
               * space.
               */
              sql_log(DEBUG_FUNC, "insufficient statement buffer size "
                "(%d of %lu bytes) for tag (%lu bytes) when preparing "
                "statement, ignoring tag '%s'", curr_avail,
                (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) taglen, tag);
            }

            /* Advance past the tag. */
            if (*tmp != '\0')
              tmp++;

          } else {
            return NULL;
          }

        } else {
          str = resolve_short_tag(cmd, *tmp);
          mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2,
            MOD_SQL_DEF_CONN_NAME, str), "sql_escapestring");
          if (check_response(mr, 0) < 0)
            return NULL;

          /* Make sure we don't write too much data. */
          taglen = strlen(mr->data);
          if (curr_avail > taglen) {
            sstrcat(curr, mr->data, curr_avail);
            curr += taglen;
            curr_avail -= taglen;

          } else {
            /* Log when this happens; it means we have more input than buffer
             * space.
             */
            sql_log(DEBUG_FUNC, "insufficient statement buffer size "
              "(%d of %lu bytes) for tag (%lu bytes) when preparing "
              "statement, ignoring tag '%%%c'", curr_avail,
              (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) taglen, *tmp);
          }

          /* Advance past the tag. */
          if (*tmp != '\0')
            tmp++;
        }

      } else {

        /* Make sure we don't try to write too much data. */
        if (curr_avail > 0) {
          *curr++ = *tmp;
          curr_avail--;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.  And break out of the processing loop.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%d of %lu bytes) for input when preparing statement, skipping",
            curr_avail, (unsigned long) SQL_MAX_STMT_LEN);
          break;
        }

        if (*tmp != '\0')
          tmp++;
      }
    }

    *curr = '\0';

  } else {
    res = buf;
  }

  return res;
}

static int _sql_strcmp(const char *s1, const char *s2) {
  if ((s1 == NULL) || (s2 == NULL))
    return 1;

  return strcmp(s1, s2);
}

static unsigned int _group_gid(const void *val) {
  if (val == NULL)
    return 0;

  return ((struct group *) val)->gr_gid;
} 

static unsigned int _group_name(const void *val) {
  char *name;
  int cnt;
  unsigned int nameval = 0;

  if (val == NULL)
    return 0;

  name = ((struct group *) val)->gr_name;

  if (name == NULL)
    return 0;

  for (cnt = 0; cnt < strlen(name); cnt++) {
    nameval += name[cnt];
  }

  return nameval;
}

static int _groupcmp(const void *val1, const void *val2) {
  if ((val1 == NULL) || (val2 == NULL))
    return 0;
  
  /* either the groupnames match or the GIDs match */
  
  if (_sql_strcmp(((struct group *) val1)->gr_name,
      ((struct group *) val2)->gr_name) == 0)
    return 1;

  if (((struct group *) val1)->gr_gid == ((struct group *) val2)->gr_gid)
    return 1;

  return 0;
}

static unsigned int _passwd_uid(const void *val) {
  if (val == NULL)
    return 0;

  return ((struct passwd *) val)->pw_uid;
} 

static unsigned int _passwd_name(const void *val) {
  char *name;
  int cnt;
  unsigned int nameval = 0;

  if (val == NULL)
    return 0;

  name = ((struct passwd *) val)->pw_name;

  if (name == NULL)
    return 0;

  for (cnt = 0; cnt < strlen(name); cnt++) {
    nameval += name[cnt];
  }

  return nameval;
}

static int _passwdcmp(const void *val1, const void *val2) {
  if ((val1 == NULL) || (val2 == NULL))
     return 0;
  
  /* either the usernames match or the UIDs match */
  if (_sql_strcmp(((struct passwd *) val1)->pw_name,
      ((struct passwd *) val2)->pw_name)  == 0)
    return 1;

  if (((struct passwd *) val1)->pw_uid == ((struct passwd *) val2)->pw_uid)
    return 1;

  return 0;
}

static void show_group(pool *p, struct group *g) {
  char *members = "";

  if (g == NULL) {
    sql_log(DEBUG_INFO, "%s", "NULL group to show_group()");
    return;
  }

  if (g->gr_mem != NULL) {
    char **member;

    member = g->gr_mem;

    while (*member != NULL) {
      pr_signals_handle();

      members = pstrcat(p, members, *members ? ", " : "", *member, NULL);
      member++;
    } 
  }

  sql_log(DEBUG_INFO, "+ grp.gr_name : %s", g->gr_name);
  sql_log(DEBUG_INFO, "+ grp.gr_gid  : %lu", (unsigned long) g->gr_gid);
  sql_log(DEBUG_INFO, "+ grp.gr_mem  : %s", members);

  return;
}

static void show_passwd(struct passwd *p) {
  if (p == NULL) {
    sql_log(DEBUG_INFO, "%s", "NULL passwd to show_passwd()");
    return;
  }

  sql_log(DEBUG_INFO, "+ pwd.pw_name  : %s", p->pw_name);
  sql_log(DEBUG_INFO, "+ pwd.pw_uid   : %lu", (unsigned long) p->pw_uid);
  sql_log(DEBUG_INFO, "+ pwd.pw_gid   : %lu", (unsigned long) p->pw_gid);
  sql_log(DEBUG_INFO, "+ pwd.pw_dir   : %s", p->pw_dir ?
    p->pw_dir : "(null)");
  sql_log(DEBUG_INFO, "+ pwd.pw_shell : %s", p->pw_shell ?
    p->pw_shell : "(null)");

  return;
}

/* _sql_addpasswd: creates a passwd and adds it to the passwd struct
 *  cache if it doesn't already exist.  Returns the created passwd
 *  struct, or the pre-existing struct if there was one.
 *
 * DOES NOT CHECK ARGUMENTS.  CALLING FUNCTIONS NEED TO MAKE SURE
 * THEY PASS VALID DATA
 */
static struct passwd *_sql_addpasswd(cmd_rec *cmd, char *username,
    char *password, uid_t uid, gid_t gid, char *shell, char *dir) {
  struct passwd *cached = NULL;
  struct passwd *pwd = NULL;

  pwd = pcalloc(cmd->tmp_pool, sizeof(struct passwd));
  pwd->pw_uid = uid;
  pwd->pw_name = username;

  /* check to make sure the entry doesn't exist in the cache */
  cached = (struct passwd *) cache_findvalue(passwd_name_cache, pwd);
  if (cached != NULL) {
    pwd = cached;
    sql_log(DEBUG_INFO, "cache hit for user '%s'", pwd->pw_name);

  } else {
    pwd = pcalloc(sql_pool, sizeof(struct passwd));

    if (username)
      pwd->pw_name = pstrdup(sql_pool, username);

    if (password)
      pwd->pw_passwd = pstrdup(sql_pool, password);
    
    pwd->pw_uid = uid;
    pwd->pw_gid = gid;
   
    if (shell) {
      pwd->pw_shell = pstrdup(sql_pool, shell);

      if (pr_table_add(session.notes, "shell", pwd->pw_shell, 0) < 0) {
        int xerrno = errno;

        if (xerrno != EEXIST) {
          pr_trace_msg(trace_channel, 8,
            "error setting 'shell' session note: %s", strerror(xerrno));
        }
      }
    }

    if (dir) {
      pwd->pw_dir = pstrdup(sql_pool, dir);

      if (pr_table_add(session.notes, "home", pwd->pw_dir, 0) < 0) {
        int xerrno = errno;

        if (xerrno != EEXIST) {
          pr_trace_msg(trace_channel, 8,
            "error setting 'home' session note: %s", strerror(xerrno));
        }
      }
    }
    
    cache_addentry(passwd_name_cache, pwd);
    cache_addentry(passwd_uid_cache, pwd);

    sql_log(DEBUG_INFO, "cache miss for user '%s'", pwd->pw_name);
    sql_log(DEBUG_INFO, "user '%s' cached", pwd->pw_name);
    show_passwd(pwd);
  }

  return pwd;
}

static int sql_getuserprimarykey(cmd_rec *cmd, const char *username) {
  sql_data_t *sd = NULL;
  modret_t *mr = NULL;
  char *key_field = NULL, *key_value = NULL;
  config_rec *c;
  void *ptr = NULL, *v = NULL;
 
  v = pr_table_get(session.notes, "sql.user-primary-key", NULL); 
  if (v != NULL) {
    /* Already have UserPrimaryKey. */
    return 0;
  }

  c = find_config(main_server->conf, CONF_PARAM, "SQLUserPrimaryKey", FALSE);
  if (c == NULL) {
    return 0;
  }

  key_field = c->argv[0];
  if (strncmp(key_field, "custom:/", 8) == 0) {
    config_rec *custom_c = NULL;
    char *named_query;

    ptr = key_field + 8;
    named_query = pstrcat(cmd->tmp_pool, "SQLNamedQuery_", ptr, NULL);

    custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
    if (custom_c == NULL) {
      sql_log(DEBUG_INFO, "error: unable to resolve custom "
        "SQLNamedQuery name '%s'", (char *) ptr);
      ptr = NULL;
    }
  }
 
  if (ptr == NULL) {
    char *where;

    where = pstrcat(cmd->tmp_pool, cmap.usrfield, " = '", username, "'", NULL);

    mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 5, MOD_SQL_DEF_CONN_NAME,
      cmap.usrtable, key_field, where, "1"), "sql_select");
    if (check_response(mr, 0) < 0) {
      return -1;
    }

    if (MODRET_HASDATA(mr)) {
      sd = (sql_data_t *) mr->data;
    }

  } else {
    mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME, ptr,
      username));
    if (check_response(mr, 0) < 0) {
      return -1;
    }

    if (MODRET_HASDATA(mr)) {
      array_header *ah = (array_header *) mr->data;
      sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

      /* Assume the query only returned 1 row. */
      sd->fnum = ah->nelts;

      sql_log(DEBUG_INFO,
        "custom SQLUserPrimaryKey query '%s' returned %d columns for user '%s'",
        ptr, sd->fnum, username);
      if (sd->fnum) {
        sd->rnum = 1;
        sd->data = (char **) ah->elts;

      } else {
        sd->rnum = 0;
        sd->data = NULL;
      }
    }
  }

  /* If we have no data...*/
  if (sd == NULL ||
      sd->rnum == 0) {
    errno = ENOENT;
    return -1;
  }

  key_value = sd->data[0];
  if (pr_table_add(session.notes, "sql.user-primary-key",
      pstrdup(session.pool, key_value), 0) < 0) {
    sql_log(DEBUG_INFO,
      "error stashing 'sql.user-primary-key' note for value '%s': %s",
      key_value, strerror(errno));
  }

  return 0;
}

static int sql_getgroupprimarykey(cmd_rec *cmd, const char *groupname) {
  sql_data_t *sd = NULL;
  modret_t *mr = NULL;
  char *key_field = NULL, *key_value = NULL;
  config_rec *c;
  void *ptr = NULL, *v = NULL;
 
  v = pr_table_get(session.notes, "sql.group-primary-key", NULL); 
  if (v != NULL) {
    /* Already have GroupPrimaryKey. */
    return 0;
  }

  c = find_config(main_server->conf, CONF_PARAM, "SQLGroupPrimaryKey", FALSE);
  if (c == NULL) {
    return 0;
  }

  key_field = c->argv[0];
  if (strncmp(key_field, "custom:/", 8) == 0) {
    config_rec *custom_c = NULL;
    char *named_query;

    ptr = key_field + 8;
    named_query = pstrcat(cmd->tmp_pool, "SQLNamedQuery_", ptr, NULL);

    custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
    if (custom_c == NULL) {
      sql_log(DEBUG_INFO, "error: unable to resolve custom "
        "SQLNamedQuery name '%s'", (char *) ptr);
      ptr = NULL;
    }
  }

  if (ptr == NULL) {
    char *where;

    where = pstrcat(cmd->tmp_pool, cmap.grpfield, " = '", groupname, "'", NULL);

    mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 5, MOD_SQL_DEF_CONN_NAME,
      cmap.grptable, key_field, where, "1"), "sql_select");
    if (check_response(mr, 0) < 0) {
      return -1;
    }

    if (MODRET_HASDATA(mr)) {
      sd = (sql_data_t *) mr->data;
    }

  } else {
    mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME, ptr,
      groupname));
    if (check_response(mr, 0) < 0) {
      return -1;
    }

    if (MODRET_HASDATA(mr)) {
      array_header *ah = (array_header *) mr->data;
      sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

      /* Assume the query only returned 1 row. */
      sd->fnum = ah->nelts;

      sql_log(DEBUG_INFO,
        "custom SQLGroupPrimaryKey query '%s' returned %d columns for "
        "group '%s'", ptr, sd->fnum, groupname);
      if (sd->fnum) {
        sd->rnum = 1;
        sd->data = (char **) ah->elts;

      } else {
        sd->rnum = 0;
        sd->data = NULL;
      }
    }
  }

  /* If we have no data...*/
  if (sd == NULL ||
      sd->rnum == 0) {
    errno = ENOENT;
    return -1;
  }

  key_value = sd->data[0];
  if (pr_table_add(session.notes, "sql.group-primary-key",
      pstrdup(session.pool, key_value), 0) < 0) {
    sql_log(DEBUG_INFO,
      "error stashing 'sql.group-primary-key' note for value '%s': %s",
      key_value, strerror(errno));
  }

  return 0;
}

static struct passwd *sql_getpasswd(cmd_rec *cmd, struct passwd *p) {
  sql_data_t *sd = NULL;
  modret_t *mr = NULL;
  struct passwd *pwd = NULL;
  char uidstr[MOD_SQL_BUFSIZE];
  char *usrwhere, *where;
  char *realname = NULL;
  int i = 0;

  char *username = NULL;
  char *password = NULL;
  char *shell = NULL;
  char *dir = NULL;
  uid_t uid = 0;
  gid_t gid = 0;

  if (p == NULL) {
    sql_log(DEBUG_WARN, "%s", "sql_getpasswd called with NULL passwd struct");
    sql_log(DEBUG_WARN, "%s", "THIS SHOULD NEVER HAPPEN");
    return NULL;
  }

  /* Check to see if the passwd already exists in one of the passwd caches.
   * Give preference to name-based lookups, as opposed to UID-based lookups.
   */
  if (p->pw_name != NULL) {
    pwd = (struct passwd *) cache_findvalue(passwd_name_cache, p);

  } else {
    pwd = (struct passwd *) cache_findvalue(passwd_uid_cache, p);
  }

  if (pwd != NULL) {
    sql_log(DEBUG_AUTH, "cache hit for user '%s'", pwd->pw_name);

    /* Check for negatively cached passwds, which will have NULL
     * passwd/home/shell.
     */
    if (pwd->pw_passwd == NULL &&
        pwd->pw_shell == NULL &&
        pwd->pw_dir == NULL) {
      sql_log(DEBUG_AUTH, "negative cache entry for user '%s'", pwd->pw_name);
      return NULL;
    }

    return pwd;
  }

  if (p->pw_name != NULL) {
    realname = p->pw_name;

    mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
      realname), "sql_escapestring");
    if (check_response(mr, 0) < 0) {
      return NULL;
    }

    username = (char *) mr->data;
    usrwhere = pstrcat(cmd->tmp_pool, cmap.usrfield, "='", username, "'", NULL);

    sql_log(DEBUG_WARN, "cache miss for user '%s'", realname);

    if (!cmap.usercustom) { 
      /* The following nested function calls may look a little strange, but
       * it is deliberate.  We want to handle any tags/variables within the
       * cmap.userwhere string (i.e. the SQLUserWhereClause directive, if
       * configured), but we do NOT want to handle any tags/variables in
       * the usrwhere variable (a string we concatenated ourselves).  The
       * usrwhere variable contains the user name, and we need to handle that
       * string as-is, lest we corrupt/change the user name.
       */

      where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, usrwhere,
        sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL), NULL);

      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 5, MOD_SQL_DEF_CONN_NAME,
        cmap.usrtable, cmap.usrfields, where, "1"), "sql_select");
      if (check_response(mr, 0) < 0) {
        return NULL;
      }

      if (MODRET_HASDATA(mr)) {
        sd = (sql_data_t *) mr->data;
      }

    } else {
      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME,
        cmap.usercustom, realname ? realname : "NULL"));

      if (check_response(mr, 0) < 0) {
        return NULL;
      }

      if (MODRET_HASDATA(mr)) {
        array_header *ah = (array_header *) mr->data;
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

        /* Assume the query only returned 1 row. */
        sd->fnum = ah->nelts;

        sql_log(DEBUG_INFO,
          "custom SQLUserInfo query '%s' returned %d columns for user '%s'",
          cmap.usercustom, sd->fnum, realname);
        if (sd->fnum) {
          sd->rnum = 1;
          sd->data = (char **) ah->elts;

        } else {
          sd->rnum = 0;
          sd->data = NULL;
        }
      }
    }

  } else {
    /* Assume we have a UID */
    memset(uidstr, '\0', sizeof(uidstr));
    snprintf(uidstr, sizeof(uidstr)-1, "%lu", (unsigned long) p->pw_uid);
    sql_log(DEBUG_WARN, "cache miss for UID '%s'", uidstr);

    if (!cmap.usercustombyid) {
      if (cmap.uidfield) {
        usrwhere = pstrcat(cmd->tmp_pool, cmap.uidfield, " = ", uidstr, NULL);

        where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2,
          usrwhere, sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL), NULL);

        mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 5,
          MOD_SQL_DEF_CONN_NAME, cmap.usrtable, cmap.usrfields, where, "1"),
          "sql_select");
        if (check_response(mr, 0) < 0) {
          return NULL;
        }

        if (MODRET_HASDATA(mr)) {
          sd = (sql_data_t *) mr->data;
        }

      } else {
        sql_log(DEBUG_WARN, "no user UID field configured, declining to "
          "lookup UID '%s'", uidstr);

        /* If no UID field has been configured, return now and let other
         * modules possibly have a chance at resolving this UID to a name.
         */
        return NULL;
      }

    } else {
      array_header *ah = NULL;

      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME,
        cmap.usercustombyid, uidstr));
      if (check_response(mr, 0) < 0) {
        return NULL;
      }

      ah = mr->data;

      sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

      /* Assume the query only return 1 row. */
      sd->fnum = ah->nelts;
      if (sd->fnum) {
        sd->rnum = 1;
        sd->data = (char **) ah->elts;

      } else {
        sd->rnum = 0;
        sd->data = NULL;
      }
    }
  }

  /* if we have no data.. */
  if (sd == NULL ||
      sd->rnum == 0) {
    if (!cmap.negative_cache) {
      return NULL;

    } else {
      /* If doing caching of negative lookups, cache this failed lookup.
       * Use the default UID and GID.
       */
      return _sql_addpasswd(cmd, username, NULL, p->pw_uid, p->pw_gid,
        NULL, NULL);
    }
  }

  i = 0;

  username = sd->data[i++];
  password = sd->data[i++];
  
  uid = cmap.defaultuid;
  if (cmap.uidfield) {
    if (sd->data[i]) {
      uid = atoi(sd->data[i++]);

    } else {
      i++;
    }
  }

  gid = cmap.defaultgid;
  if (cmap.gidfield) {
    if (sd->data[i]) {
      gid = atoi(sd->data[i++]);

    } else {
      i++;
    }
  }

  dir = cmap.defaulthomedir;
  if (sd->data[i]) {
    if (strcmp(sd->data[i], "") == 0 ||
        strcmp(sd->data[i], "NULL") == 0) {

      /* Leave dir pointing to the SQLDefaultHomedir, if any. */
      i++;

    } else {
      dir = sd->data[i++];
    }
  }

  if (cmap.shellfield) {
    if (sd->fnum-1 < i ||
        !sd->data[i]) {

      /* Make sure that, if configured, the shell value is valid, and scream
       * if it is not.
       */
      sql_log(DEBUG_WARN, "NULL shell column value");
      shell = NULL;

    } else {
      shell = sd->data[i];
    }

  } else {
    shell = NULL;
  }

  if (uid < cmap.minuseruid) {
    sql_log(DEBUG_INFO, "user UID %lu below SQLMinUserUID %lu, using "
      "SQLDefaultUID %lu", (unsigned long) uid, (unsigned long) cmap.minuseruid,
      (unsigned long) cmap.defaultuid);
    uid = cmap.defaultuid;
  }

  if (gid < cmap.minusergid) {
    sql_log(DEBUG_INFO, "user GID %lu below SQLMinUserGID %lu, using "
      "SQLDefaultGID %lu", (unsigned long) gid, (unsigned long) cmap.minusergid,
      (unsigned long) cmap.defaultgid);
    gid = cmap.defaultgid;
  }

  return _sql_addpasswd(cmd, username, password, uid, gid, shell, dir);
}

/* _sql_addgroup: creates a group and adds it to the group struct
 *  cache if it doesn't already exist.  Returns the created group
 *  struct, or the pre-existing struct if there was one.
 *
 * DOES NOT CHECK ARGUMENTS.  CALLING FUNCTIONS NEED TO MAKE SURE
 * THEY PASS VALID DATA
 */
static struct group *_sql_addgroup(cmd_rec *cmd, char *groupname, gid_t gid,
    array_header *ah) {
  struct group *cached = NULL;
  struct group *grp = NULL;

  grp = pcalloc(cmd->tmp_pool, sizeof(struct group));
  grp->gr_gid = gid;
  grp->gr_name = groupname;

  /* check to make sure the entry doesn't exist in the cache */
  if ((cached = (struct group *) cache_findvalue(group_name_cache, grp)) != NULL) {
    grp = cached;
    sql_log(DEBUG_INFO, "cache hit for group '%s'", grp->gr_name);

  } else {
    grp = pcalloc(sql_pool, sizeof(struct group));

    if (groupname) {
      grp->gr_name = pstrdup(sql_pool, groupname);

      if (pr_table_add(session.notes, "primary-group", grp->gr_name, 0) < 0) {
        int xerrno = errno;

        if (xerrno != EEXIST) {
          pr_trace_msg(trace_channel, 8,
            "error setting 'primary-group' session note: %s", strerror(xerrno));
        }
      }
    }

    grp->gr_gid = gid;

    if (ah) {
      register unsigned int i;

      /* finish filling in the group */
      grp->gr_mem = (char **) pcalloc(sql_pool,
        sizeof(char *) * (ah->nelts + 1));

      for (i = 0; i < ah->nelts; i++) {
        grp->gr_mem[i] = pstrdup(sql_pool, ((char **) ah->elts)[i]);
      }

      grp->gr_mem[i] = NULL;
    }

    cache_addentry(group_name_cache, grp);
    cache_addentry(group_gid_cache, grp);

    sql_log(DEBUG_INFO, "cache miss for group '%s'", grp->gr_name);
    sql_log(DEBUG_INFO, "group '%s' cached", grp->gr_name);
    show_group(cmd->tmp_pool, grp);
  }

  return grp;
}

static struct group *sql_getgroup(cmd_rec *cmd, struct group *g) {
  struct group *grp = NULL;
  modret_t *mr = NULL;
  int cnt = 0;
  sql_data_t *sd = NULL;
  char *groupname = NULL;
  char gidstr[MOD_SQL_BUFSIZE] = {'\0'};
  char **rows = NULL;
  int numrows = 0;
  array_header *ah = NULL;
  char *members = NULL;
  char *member = NULL;
  char *grpwhere;
  char *where;
  char *iterator;

  gid_t gid = 0;
  
  if (g == NULL) {
    sql_log(DEBUG_WARN, "%s", "sql_getgroup called with NULL group struct");
    sql_log(DEBUG_WARN, "%s", "THIS SHOULD NEVER HAPPEN");
    return NULL;
  }

  /* check to see if the group already exists in one of the group caches */
  if (((grp = (struct group *) cache_findvalue(group_name_cache, g)) != NULL) ||
      ((grp = (struct group *) cache_findvalue(group_gid_cache, g)) != NULL)) {
    sql_log(DEBUG_AUTH, "cache hit for group '%s'", grp->gr_name);

    /* Check for negatively cached groups, which will have NULL gr_mem. */
    if (!grp->gr_mem) {
      sql_log(DEBUG_AUTH, "negative cache entry for group '%s'", grp->gr_name);
      return NULL;
    }

    return grp;
  }

  if (g->gr_name != NULL) {
    groupname = g->gr_name;
    sql_log(DEBUG_WARN, "cache miss for group '%s'", groupname);

  } else {
    /* Get groupname from GID */
    snprintf(gidstr, MOD_SQL_BUFSIZE, "%lu", (unsigned long) g->gr_gid);

    sql_log(DEBUG_WARN, "cache miss for GID '%s'", gidstr);

    if (!cmap.groupcustombyid) {
      if (cmap.grpgidfield) {
        grpwhere = pstrcat(cmd->tmp_pool, cmap.grpgidfield, " = ", gidstr,
          NULL);

      } else {
        sql_log(DEBUG_WARN, "no group GID field configured, declining to "
          "lookup GID '%s'", gidstr);

        /* If no GID field has been configured, return now and let other
         * modules possibly have a chance at resolving this GID to a name.
         */
        return NULL;
      }

      where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, grpwhere,
        sql_prepare_where(0, cmd, 1, cmap.groupwhere, NULL), NULL);

      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 5, MOD_SQL_DEF_CONN_NAME,
        cmap.grptable, cmap.grpfield, where, "1"), "sql_select");
      if (check_response(mr, 0) < 0) {
        return NULL;
      }

      sd = (sql_data_t *) mr->data;

    } else {
      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME,
        cmap.groupcustombyid, gidstr));
      if (check_response(mr, 0) < 0) {
        return NULL;
      }

      ah = mr->data;

      sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

      /* Assume the query only return 1 row. */
      sd->fnum = ah->nelts;
      if (sd->fnum) {
        sd->rnum = 1;
        sd->data = (char **) ah->elts;

      } else {
        sd->rnum = 0;
        sd->data = NULL;
      }
    }

    /* If we have no data.. */
    if (sd->rnum == 0)
      return NULL;

    groupname = sd->data[0];
  }

  if (!cmap.groupcustombyname) {
    grpwhere = pstrcat(cmd->tmp_pool, cmap.grpfield, " = '", groupname, "'",
      NULL);

    where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, grpwhere,
      sql_prepare_where(0, cmd, 1, cmap.groupwhere, NULL), NULL);

    mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
      cmap.grptable, cmap.grpfields, where), "sql_select");
    if (check_response(mr, 0) < 0) {
      return NULL;
    }
 
    sd = (sql_data_t *) mr->data;

  } else {
    mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME,
      cmap.groupcustombyname, groupname ? groupname : "NULL"));
    if (check_response(mr, 0) < 0) {
      return NULL;
    }

    ah = mr->data;
    sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));
 
    /* Assume the query only returned 1 row. */
    sd->fnum = ah->nelts;

    if (sd->fnum) {
      sd->rnum = 1;
      sd->data = (char **) ah->elts;

    } else {
      sd->rnum = 0;
      sd->data = NULL;
    }
  }

  /* if we have no data.. */
  if (sd->rnum == 0) {
    if (!cmap.negative_cache) {
      return NULL;

    } else {

      /* If doing caching of negative lookups, cache this failed lookup. */
      return _sql_addgroup(cmd, groupname, g->gr_gid, NULL);
    }
  }
 
  rows = sd->data;
  numrows = sd->rnum;
  
  gid = (gid_t) strtoul(rows[1], NULL, 10);
  
  /* Painful.. we need to walk through the returned rows and fill in our
   * members. Every third element in a row is a member field, and every
   * member field can have multiple members.
   */
  
  ah = make_array(cmd->tmp_pool, 10, sizeof(char *));
  
  for (cnt = 0; cnt < numrows; cnt++) {
    members = rows[(cnt * 3) + 2];
    iterator = members;
   
    pr_signals_handle();
 
    /* If the row is null, continue.. */
    if (members == NULL)
      continue;
    
    /* For each member in the list, toss 'em into the array.  no
     * need to copy the string -- _sql_addgroup will do it for us 
     */
    for (member = strsep(&iterator, ","); member;
        member = strsep(&iterator, ",")) {
      if (*member == '\0') {
        continue;
      }

      *((char **) push_array(ah)) = member;
    }      
  }
  
  return _sql_addgroup(cmd, groupname, gid, ah);
}

static void _setstats(cmd_rec *cmd, int fstor, int fretr, int bstor,
    int bretr) {
  /*
   * if anyone has a better way of doing this, let me know.. 
   */
  char query[256] = { '\0' };
  char *usrwhere, *where;
  modret_t *mr = NULL;

  snprintf(query, sizeof(query),
           "%s = %s + %i, %s = %s + %i, %s = %s + %i, %s = %s + %i",
           cmap.sql_fstor, cmap.sql_fstor, fstor,
           cmap.sql_fretr, cmap.sql_fretr, fretr,
           cmap.sql_bstor, cmap.sql_bstor, bstor,
	   cmap.sql_bretr, cmap.sql_bretr, bretr);

  usrwhere = pstrcat(cmd->tmp_pool, cmap.usrfield, " = '", _sql_realuser(cmd),
    "'", NULL);

  where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, usrwhere,
    sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL), NULL);

  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
    cmap.usrtable, query, where), "sql_update");
  (void) check_response(mr, 0);
}

static int sql_getgroups(cmd_rec *cmd) {
  struct passwd *pw = NULL, lpw;
  struct group *grp, lgr;
  char *grpwhere = NULL, *where = NULL;
  sql_data_t *sd = NULL;
  modret_t *mr = NULL;
  array_header *gids = NULL, *groups = NULL;
  char *name = cmd->argv[0], *username = NULL;
  int argc, numrows = 0, res = -1;
  register unsigned int i = 0;

  /* Check for NULL values */
  if (cmd->argv[1]) {
    gids = (array_header *) cmd->argv[1];
  }

  if (cmd->argv[2]) {
    groups = (array_header *) cmd->argv[2];
  }

  lpw.pw_uid = -1;
  lpw.pw_name = name;

  /* Now that we have the pointers for the lists, tweak the argc field
   * before passing this cmd_rec on, lest we try to resolve some variable
   * like %r which will assume that all of the cmd_rec args are strings, as
   * from the client.
   */
  argc = cmd->argc;
  cmd->argc = 1;

  /* Retrieve the necessary info */
  if (!name ||
      !(pw = sql_getpasswd(cmd, &lpw))) {
    cmd->argc = argc;
    return -1;
  }

  /* Populate the first group ID and name */
  if (gids) {
    *((gid_t *) push_array(gids)) = pw->pw_gid;
  }

  lgr.gr_gid = pw->pw_gid;
  lgr.gr_name = NULL;

  if (groups &&
      (grp = sql_getgroup(cmd, &lgr)) != NULL) {
    *((char **) push_array(groups)) = pstrdup(permanent_pool, grp->gr_name);
  }

  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
    name), "sql_escapestring");
  if (check_response(mr, 0) < 0) {
    cmd->argc = argc;
    return -1;
  }

  username = (char *) mr->data;

  if (!cmap.groupcustommembers) {
    if (!(pr_sql_opts & SQL_OPT_USE_NORMALIZED_GROUP_SCHEMA)) {

      /* Use a SELECT with a LIKE clause:
       *
       *  SELECT groupname,gid,members FROM groups
       *    WHERE members LIKE '%,<user>,%' OR LIKE '<user>,%' OR LIKE '%,<user>';
       */

      grpwhere = pstrcat(cmd->tmp_pool,
        cmap.grpmembersfield, " = '", username, "' OR ",
        cmap.grpmembersfield, " LIKE '", username, ",%' OR ",
        cmap.grpmembersfield, " LIKE '%,", username, "' OR ",
        cmap.grpmembersfield, " LIKE '%,", username, ",%'", NULL);

    } else {
      /* Use a single SELECT:
       *
       *  SELECT groupname,gid,members FROM groups WHERE members = <user>';
       */
      grpwhere = pstrcat(cmd->tmp_pool,
        cmap.grpmembersfield, " = '", username, "'", NULL);
    }

    where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, grpwhere,
      sql_prepare_where(0, cmd, 1, cmap.groupwhere, NULL), NULL);
  
    mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
      cmap.grptable, cmap.grpfields, where), "sql_select");
    if (check_response(mr, 0) < 0) {
      cmd->argc = argc;
      return -1;
    }
 
    sd = (sql_data_t *) mr->data;

  } else {
    array_header *ah;

    /* The username has been escaped according to the backend database' rules
     * at this point.
     */
    mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 3, MOD_SQL_DEF_CONN_NAME,
      cmap.groupcustommembers, username));
    if (check_response(mr, 0) < 0) {
      cmd->argc = argc;
      return -1;
    }

    ah = mr->data;
    sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

    /* Assume the query returned N rows, 3 columns per row. */
    if (ah->nelts % 3 == 0) {
      sd->fnum = 3;
      sd->rnum = ah->nelts / 3;

      if (sd->rnum > 0) {
        sd->data = (char **) ah->elts;
      }

    } else {
      sql_log(DEBUG_INFO, "wrong number of columns (%d) returned by custom SQLGroupInfo members query, ignoring results", ah->nelts % 3);
      sd->rnum = 0;
      sd->data = NULL;
    }
  }

  /* If we have no data... */
  if (sd->rnum == 0) {
    cmd->argc = argc;
    return -1;
  }

  numrows = sd->rnum;

  for (i = 0; i < numrows; i++) {
    char *groupname = sd->data[(i * 3)];
    gid_t gid = (gid_t) atoi(sd->data[(i * 3) +1]);
    char *memberstr = sd->data[(i * 3) + 2], *member = NULL;
    array_header *members = make_array(cmd->tmp_pool, 2, sizeof(char *));

    *((gid_t *) push_array(gids)) = gid;
    *((char **) push_array(groups)) = pstrdup(permanent_pool, groupname);

    /* For each member in the list, toss 'em into the array.  no
     * need to copy the string -- _sql_addgroup will do it for us
     */
    for (member = strsep(&memberstr, ","); member;
        member = strsep(&memberstr, ",")) {
      if (*member == '\0') {
        continue;
      }

      *((char **) push_array(members)) = member;
    }

    /* Add this group data to the group cache. */
    _sql_addgroup(cmd, groupname, gid, members);
  }

  if (gids &&
      gids->nelts > 0) {
    res = gids->nelts;

  } else if (groups &&
           groups->nelts) {
    res = groups->nelts;
  }

  cmd->argc = argc;
  return res;
}

/* Command handlers
 */

MODRET sql_pre_dele(cmd_rec *cmd) {
  char *path;

  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  sql_dele_filesz = 0;

  path = dir_canonical_path(cmd->tmp_pool,
    pr_fs_decode_path(cmd->tmp_pool, cmd->arg));
  if (path) {
    struct stat st;

    /* Briefly cache the size of the file being deleted, so that it can be
     * logged properly using %b.
     */
    pr_fs_clear_cache();
    if (pr_fsio_stat(path, &st) < 0) {
      sql_log(DEBUG_INFO, "%s: unable to stat '%s': %s", cmd->argv[0],
        path, strerror(errno));
    
    } else
      sql_dele_filesz = st.st_size;
  }

  return PR_DECLINED(cmd);
}

MODRET sql_pre_pass(cmd_rec *cmd) {
  config_rec *c = NULL;
  char *user = NULL;

  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> sql_pre_pass");

  user = pr_table_get(session.notes, "mod_auth.orig-user", NULL);
  if (user) {
    config_rec *anon_config;

    /* Use the looked-up user name to determine whether this is to be
     * an anonymous session.
     */
    anon_config = pr_auth_get_anon_config(cmd->pool, &user, NULL, NULL);

    c = find_config(anon_config ? anon_config->subset : main_server->conf,
      CONF_PARAM, "SQLEngine", FALSE);
    if (c != NULL) {
      cmap.engine = *((int *) c->argv[0]);
    }

  } else {
    /* Just assume the vhost config. */
    c = find_config(main_server->conf, CONF_PARAM, "SQLEngine", FALSE);
    if (c != NULL) {
      cmap.engine = *((int *) c->argv[0]);
    }
  }

  sql_log(DEBUG_FUNC, "%s", "<<< sql_pre_pass");
  return PR_DECLINED(cmd);
}

MODRET sql_post_pass(cmd_rec *cmd) {
  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  sql_getuserprimarykey(cmd, session.user);
  sql_getgroupprimarykey(cmd, session.group);

  return PR_DECLINED(cmd);
}

MODRET sql_post_stor(cmd_rec *cmd) {
  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> sql_post_stor");

  if (cmap.sql_fstor)
    _setstats(cmd, 1, 0, session.xfer.total_bytes, 0);

  sql_log(DEBUG_FUNC, "%s", "<<< sql_post_stor");
  return PR_DECLINED(cmd);
}

MODRET sql_post_retr(cmd_rec *cmd) {
  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> sql_post_retr");

  if (cmap.sql_fretr)
    _setstats(cmd, 0, 1, 0, session.xfer.total_bytes);

  sql_log(DEBUG_FUNC, "%s", "<<< sql_post_retr");
  return PR_DECLINED(cmd);
}

static const char *resolve_long_tag(cmd_rec *cmd, char *tag) {
  const char *long_tag = NULL;
  size_t taglen;

  if (strncmp(tag, "uid", 4) == 0) {
    char buf[64];

    memset(buf, '\0', sizeof(buf));
    snprintf(buf, sizeof(buf)-1, "%lu", (unsigned long) session.login_uid);
    
    long_tag = pstrdup(cmd->tmp_pool, buf);
  }

  if (long_tag == NULL &&
      strncmp(tag, "gid", 4) == 0) {
    char buf[64];

    memset(buf, '\0', sizeof(buf));
    snprintf(buf, sizeof(buf)-1, "%lu", (unsigned long) session.login_gid);
    
    long_tag = pstrdup(cmd->tmp_pool, buf);
  }

  if (long_tag == NULL &&
      strncasecmp(tag, "iso8601", 8) == 0) {
    char buf[32];
    struct timeval now;
    struct tm *tm;
    size_t len;
    unsigned long millis;

    memset(buf, '\0', sizeof(buf));
    gettimeofday(&now, NULL);
    tm = pr_localtime(NULL, (const time_t *) &(now.tv_sec));

    len = strftime(buf, sizeof(buf)-1, "%Y-%m-%d %H:%M:%S", tm);

    /* Convert microsecs to millisecs. */
    millis = now.tv_usec / 1000;

    snprintf(buf + len, sizeof(buf) - len, ",%03lu", millis);
    long_tag = pstrdup(cmd->tmp_pool, buf);
  }

  if (long_tag == NULL &&
      strncmp(tag, "microsecs", 10) == 0) {
    char buf[7];
    struct timeval now;

    memset(buf, '\0', sizeof(buf));
    gettimeofday(&now, NULL); 

    snprintf(buf, sizeof(buf), "%06lu", (unsigned long) now.tv_usec);
    long_tag = pstrdup(cmd->tmp_pool, buf);
  }

  if (long_tag == NULL &&
      strncmp(tag, "millisecs", 10) == 0) {
    char buf[4];
    struct timeval now;
    unsigned long millis;

    memset(buf, '\0', sizeof(buf));
    gettimeofday(&now, NULL);

    /* Convert microsecs to millisecs. */
    millis = now.tv_usec / 1000;

    snprintf(buf, sizeof(buf), "%03lu", millis);
    long_tag = pstrdup(cmd->tmp_pool, buf);
  }

  if (long_tag == NULL &&
      strncmp(tag, "protocol", 9) == 0) {
    long_tag = pr_session_get_protocol(0);
  }

  taglen = strlen(tag);

  if (long_tag == NULL &&
      taglen > 5 &&
      strncmp(tag, "env:", 4) == 0) {
    char *env;

    env = pr_env_get(cmd->tmp_pool, tag + 4);
    long_tag = pstrdup(cmd->tmp_pool, env ? env : "");
  }

  if (long_tag == NULL &&
      taglen > 5 &&
      strncmp(tag, "note:", 5) == 0) {
    char *key = NULL, *note = NULL;

    key = tag + 5;

    /* Check first in the command.notes table, then in the session.notes. */
    note = pr_table_get(cmd->notes, key, NULL);
    if (note == NULL) {
      note = pr_table_get(session.notes, key, NULL);
    }

    long_tag = pstrdup(cmd->tmp_pool, note ? note : "");
  }

  if (long_tag == NULL &&
      taglen > 6 &&
      strncmp(tag, "time:", 5) == 0) {
    char time_str[128], *fmt;
    time_t now;
    struct tm *time_info;

    fmt = pstrdup(cmd->tmp_pool, tag + 5);
    now = time(NULL);
    time_info = pr_localtime(NULL, &now);

    memset(time_str, 0, sizeof(time_str));
    strftime(time_str, sizeof(time_str), fmt, time_info);

    long_tag = pstrdup(cmd->tmp_pool, time_str);
  }

  if (long_tag == NULL &&
      strncmp(tag, "basename", 9) == 0) {
    const char *path = NULL;

    if (pr_cmd_cmp(cmd, PR_CMD_RNTO_ID) == 0) {
      path = pr_fs_decode_path(cmd->tmp_pool, cmd->arg);

    } else if (pr_cmd_cmp(cmd, PR_CMD_RETR_ID) == 0) {
      path = pr_table_get(cmd->notes, "mod_xfer.retr-path", NULL);

    } else if (pr_cmd_cmp(cmd, PR_CMD_APPE_ID) == 0 ||
               pr_cmd_cmp(cmd, PR_CMD_STOR_ID) == 0) {
      path = pr_table_get(cmd->notes, "mod_xfer.store-path", NULL);

    } else if (session.xfer.p &&
               session.xfer.path) {
      path = session.xfer.path;

    } else if (pr_cmd_cmp(cmd, PR_CMD_CDUP_ID) == 0 ||
               pr_cmd_cmp(cmd, PR_CMD_PWD_ID) == 0 ||
               pr_cmd_cmp(cmd, PR_CMD_XCUP_ID) == 0 ||
               pr_cmd_cmp(cmd, PR_CMD_XPWD_ID) == 0) {
      path = pr_fs_getcwd();

    } else if (pr_cmd_cmp(cmd, PR_CMD_CWD_ID) == 0 ||
               pr_cmd_cmp(cmd, PR_CMD_XCWD_ID) == 0) {

      if (session.chroot_path) {
        /* Chrooted session. */
        path = strcmp(pr_fs_getvwd(), "/") ? pr_fs_getvwd() :
          session.chroot_path;

      } else {
        /* Non-chrooted session. */
        path = pr_fs_getcwd();
      }

    } else if (pr_cmd_cmp(cmd, PR_CMD_SITE_ID) == 0 &&
               (strncasecmp(cmd->argv[1], "CHGRP", 6) == 0 ||
                strncasecmp(cmd->argv[1], "CHMOD", 6) == 0 ||
                strncasecmp(cmd->argv[1], "UTIME", 6) == 0)) {
      register unsigned int i;
      char *tmp = "";

      for (i = 3; i <= cmd->argc-1; i++) {
        tmp = pstrcat(cmd->tmp_pool, tmp, *tmp ? " " : "",
          pr_fs_decode_path(cmd->tmp_pool, cmd->argv[i]), NULL);
      }

      path = tmp;

    } else {
      /* Some commands (i.e. DELE, MKD, RMD, XMKD, and XRMD) have associated
       * filenames that are not stored in the session.xfer structure; these
       * should be expanded properly as well.
       */
      if (pr_cmd_cmp(cmd, PR_CMD_DELE_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_LIST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MDTM_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MKD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MLSD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MLST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_NLST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_RMD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XMKD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XRMD_ID) == 0) {
        path = pr_fs_decode_path(cmd->tmp_pool, cmd->arg);

      } else if (pr_cmd_cmp(cmd, PR_CMD_MFMT_ID) == 0) {
        /* MFMT has, as its filename, the second argument. */
        path = pr_fs_decode_path(cmd->tmp_pool, cmd->argv[2]);
      }
    }

    if (path != NULL) {
      char *ptr = NULL;

      ptr = strrchr(path, '/');
      if (ptr != NULL) {
        if (ptr != path) {
          long_tag = pstrdup(cmd->tmp_pool, ptr + 1);

        } else if (*(ptr+1) != '\0') {
          long_tag = pstrdup(cmd->tmp_pool, ptr + 1);

        } else {
          long_tag = pstrdup(cmd->tmp_pool, path);
        }

      } else {
        long_tag = pstrdup(cmd->tmp_pool, path);
      }
    }
  }

  if (long_tag == NULL &&
      strncmp(tag, "transfer-failure", 17) == 0) {

    /* If the current command is one that incurs a data transfer, then we
     * need to do more work.  If not, it's an easy substitution.
     */
    if (session.curr_cmd_id == PR_CMD_APPE_ID ||
        session.curr_cmd_id == PR_CMD_LIST_ID ||
        session.curr_cmd_id == PR_CMD_MLSD_ID ||
        session.curr_cmd_id == PR_CMD_NLST_ID ||
        session.curr_cmd_id == PR_CMD_RETR_ID ||
        session.curr_cmd_id == PR_CMD_STOR_ID ||
        session.curr_cmd_id == PR_CMD_STOU_ID) {
      const char *proto;

      proto = pr_session_get_protocol(0);

      if (strncmp(proto, "ftp", 4) == 0 ||
          strncmp(proto, "ftps", 5) == 0) {

        if (XFER_ABORTED) {
          long_tag = pstrdup(cmd->tmp_pool, "-");

        } else {
          int res;
          char *resp_code = NULL, *resp_msg = NULL;

          /* Get the last response code/message.  We use heuristics here to
           * determine when to use "failed" versus "success".
           */
          res = pr_response_get_last(cmd->tmp_pool, &resp_code, &resp_msg);
          if (res == 0 &&
              resp_code != NULL) {
            if (*resp_code != '2' &&
                *resp_code != '1') {
              char *ptr;

              /* Parse out/prettify the resp_msg here */
              ptr = strchr(resp_msg, '.');
              if (ptr != NULL) {
                long_tag = pstrdup(cmd->tmp_pool, ptr + 2);

              } else {
                long_tag = pstrdup(cmd->tmp_pool, resp_msg);
              }

            } else {
              long_tag = pstrdup(cmd->tmp_pool, "-");
            }

          } else {
            long_tag = pstrdup(cmd->tmp_pool, "-");
          }
        }

      } else {
        /* Currently, for failed SFTP/SCP transfers, we can't properly
         * populate the failure reason.  Maybe in the future.
         */
        long_tag = pstrdup(cmd->tmp_pool, "-");
      }

    } else {
      long_tag = pstrdup(cmd->tmp_pool, "-");
    }
  }

  if (long_tag == NULL &&
      strncmp(tag, "transfer-status", 16) == 0) {

    /* If the current command is one that incurs a data transfer, then we
     * need to do more work.  If not, it's an easy substitution.
     */
    if (session.curr_cmd_id == PR_CMD_ABOR_ID ||
        session.curr_cmd_id == PR_CMD_APPE_ID ||
        session.curr_cmd_id == PR_CMD_LIST_ID ||
        session.curr_cmd_id == PR_CMD_MLSD_ID ||
        session.curr_cmd_id == PR_CMD_NLST_ID ||
        session.curr_cmd_id == PR_CMD_RETR_ID ||
        session.curr_cmd_id == PR_CMD_STOR_ID ||
        session.curr_cmd_id == PR_CMD_STOU_ID) {
      const char *proto;

      proto = pr_session_get_protocol(0);

      if (strncmp(proto, "ftp", 4) == 0 ||
          strncmp(proto, "ftps", 5) == 0) {

        if (!(XFER_ABORTED)) {
          int res;
          char *resp_code = NULL, *resp_msg = NULL;

          /* Get the last response code/message.  We use heuristics here to
           * determine when to use "failed" versus "success".
           */
          res = pr_response_get_last(cmd->tmp_pool, &resp_code, &resp_msg);
          if (res == 0 &&
              resp_code != NULL) {
            if (*resp_code == '2') {
              if (pr_cmd_cmp(cmd, PR_CMD_ABOR_ID) != 0) {
                long_tag = pstrdup(cmd->tmp_pool, "success");

              } else {
                /* We're handling the ABOR command, so obviously the value
                 * should be 'cancelled'.
                 */
                long_tag = pstrdup(cmd->tmp_pool, "cancelled");
              }

            } else if (*resp_code == '1') {
              /* If the first digit of the response code is 1, then the response
               * code (for a data transfer command) is probably 150, which means
               * that the transfer was still in progress (didn't complete with
               * a 2xx/4xx response code) when we are called here, which in turn
               * means a timeout kicked in.
               */
              long_tag = pstrdup(cmd->tmp_pool, "timeout");

            } else {
              long_tag = pstrdup(cmd->tmp_pool, "failed");
            }

          } else {
            long_tag = pstrdup(cmd->tmp_pool, "success");
          }

        } else {
          long_tag = pstrdup(cmd->tmp_pool, "cancelled");
        }

      } else {
        /* mod_sftp stashes a note for us in the command notes if the
         * transfer failed.
         */
        char *status;

        status = pr_table_get(cmd->notes, "mod_sftp.file-status", NULL);
        if (status == NULL) {
          long_tag = pstrdup(cmd->tmp_pool, "success");

        } else {
          long_tag = pstrdup(cmd->tmp_pool, "failed");
        }
      }

    } else {
      long_tag = pstrdup(cmd->tmp_pool, "-");
    }
  }

  pr_trace_msg(trace_channel, 15, "returning long tag '%s' for tag '%s'",
    long_tag ? long_tag : "<null>", tag);
  return long_tag;
}

static int resolve_numeric_tag(cmd_rec *cmd, char *tag) {
  int num = -1;
  char *endp = NULL;

  num = strtol(tag, &endp, 10);
  if (*endp != '\0')
    return -1;

  if (num < 0 || (cmd->argc - 3) < num)
    return -1;

  return num;
}

static char *resolve_short_tag(cmd_rec *cmd, char tag) {
  char arg[PR_TUNABLE_PATH_MAX+1], *argp = NULL, *short_tag = NULL;

  memset(arg, '\0', sizeof(arg));

  switch (tag) {
    case 'A': {
      char *pass;

      argp = arg;
      pass = pr_table_get(session.notes, "mod_auth.anon-passwd", NULL);
      if (!pass)
	pass = "UNKNOWN";
      
      sstrncpy(argp, pass, sizeof(arg));
      break;
    }

    case 'a':
      argp = arg;
      sstrncpy(argp, pr_netaddr_get_ipstr(pr_netaddr_get_sess_remote_addr()),
        sizeof(arg));
      break;

    case 'b':
      argp = arg;
      if (session.xfer.p) {
        snprintf(argp, sizeof(arg), "%" PR_LU,
          (pr_off_t) session.xfer.total_bytes);

      } else if (pr_cmd_cmp(cmd, PR_CMD_DELE_ID) == 0) {
        snprintf(argp, sizeof(arg), "%" PR_LU, (pr_off_t) sql_dele_filesz);

      } else {
        sstrncpy(argp, "0", sizeof(arg));
      }
      break;

    case 'c':
      argp = arg;
      sstrncpy(argp, session.conn_class ? session.conn_class->cls_name : "-",
        sizeof(arg));
      break;

    case 'd':
      argp = arg;

      if (pr_cmd_cmp(cmd, PR_CMD_CDUP_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_CWD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_LIST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MLSD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MKD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_NLST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_RMD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XCWD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XCUP_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XMKD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XRMD_ID) == 0) {
        char *tmp = strrchr(cmd->arg, '/');

        if (tmp != NULL) {
          if (tmp != cmd->arg) {
            sstrncpy(argp, tmp + 1, sizeof(arg));

          } else if (*(tmp + 1) != '\0') {
            sstrncpy(argp, tmp + 1, sizeof(arg));

          } else {
            sstrncpy(argp, cmd->arg, sizeof(arg));
          }

        } else {
          sstrncpy(argp, cmd->arg, sizeof(arg));
        }

      } else {
        sstrncpy(argp, pr_fs_getvwd(), sizeof(arg));
      }
      break;

    case 'D':
      argp = arg;

      if (pr_cmd_cmp(cmd, PR_CMD_CDUP_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_LIST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MLSD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MKD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_NLST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_RMD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XCUP_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XMKD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_XRMD_ID) == 0) {
        sstrncpy(argp, dir_abs_path(cmd->tmp_pool, cmd->arg, TRUE),
          sizeof(arg));

      } else if (pr_cmd_cmp(cmd, PR_CMD_CWD_ID) == 0 ||
                 pr_cmd_cmp(cmd, PR_CMD_XCWD_ID) == 0) {

        /* Note: by this point in the dispatch cycle, the current working
         * directory has already been changed.  For the CWD/XCWD commands,
         * this means that dir_abs_path() may return an improper path,
         * with the target directory being reported twice.  To deal with this,
         * don't use dir_abs_path(), and use pr_fs_getvwd()/pr_fs_getcwd()
         * instead.
         */

        if (session.chroot_path) {
          /* Chrooted session. */
          sstrncpy(arg, strcmp(pr_fs_getvwd(), "/") ?
            pdircat(cmd->tmp_pool, session.chroot_path, pr_fs_getvwd(), NULL) :
            session.chroot_path, sizeof(arg));

        } else {

          /* Non-chrooted session. */
          sstrncpy(arg, pr_fs_getcwd(), sizeof(arg));
        }

      } else {
        sstrncpy(argp, "", sizeof(arg));
      }
      break;

    case 'E': {
      const char *reason_str;
      char *details = NULL;

      argp = arg;

      reason_str = pr_session_get_disconnect_reason(&details);
      sstrncpy(argp, reason_str, sizeof(arg));
      if (details != NULL) {
        sstrcat(argp, ": ", sizeof(arg));
        sstrcat(argp, details, sizeof(arg));
      }

      break;
    }

    case 'f':
      argp = arg;

      if (pr_cmd_cmp(cmd, PR_CMD_RNTO_ID) == 0) {
        sstrncpy(argp, dir_abs_path(cmd->tmp_pool, cmd->arg, TRUE),
          sizeof(arg));

      } else if (pr_cmd_cmp(cmd, PR_CMD_RETR_ID) == 0) {
        char *path;

        path = pr_table_get(cmd->notes, "mod_xfer.retr-path", NULL);
        sstrncpy(arg, dir_abs_path(cmd->tmp_pool, path, TRUE), sizeof(arg));

      } else if (pr_cmd_cmp(cmd, PR_CMD_APPE_ID) == 0 ||
                 pr_cmd_cmp(cmd, PR_CMD_STOR_ID) == 0) {
        char *path;

        path = pr_table_get(cmd->notes, "mod_xfer.store-path", NULL);
        sstrncpy(arg, dir_abs_path(cmd->tmp_pool, path, TRUE), sizeof(arg));

      } else if (session.xfer.p &&
                 session.xfer.path) {
        sstrncpy(argp, dir_abs_path(cmd->tmp_pool, session.xfer.path, TRUE),
          sizeof(arg));

      } else if (pr_cmd_cmp(cmd, PR_CMD_CDUP_ID) == 0 ||
                 pr_cmd_cmp(cmd, PR_CMD_PWD_ID) == 0 ||
                 pr_cmd_cmp(cmd, PR_CMD_XCUP_ID) == 0 ||
                 pr_cmd_cmp(cmd, PR_CMD_XPWD_ID) == 0) {
        sstrncpy(argp, dir_abs_path(cmd->tmp_pool, pr_fs_getcwd(), TRUE),
          sizeof(arg));

      } else if (pr_cmd_cmp(cmd, PR_CMD_CWD_ID) == 0 ||
                 pr_cmd_cmp(cmd, PR_CMD_XCWD_ID) == 0) {

        /* Note: by this point in the dispatch cycle, the current working
         * directory has already been changed.  For the CWD/XCWD commands,
         * this means that dir_abs_path() may return an improper path,
         * with the target directory being reported twice.  To deal with this,
         * don't use dir_abs_path(), and use pr_fs_getvwd()/pr_fs_getcwd()
         * instead.
         */
        if (session.chroot_path) {
          /* Chrooted session. */
          sstrncpy(arg, strcmp(pr_fs_getvwd(), "/") ?
            pdircat(cmd->tmp_pool, session.chroot_path, pr_fs_getvwd(), NULL) :
            session.chroot_path, sizeof(arg));

        } else {
          /* Non-chrooted session. */
          sstrncpy(arg, pr_fs_getcwd(), sizeof(arg));
        }

      } else if (pr_cmd_cmp(cmd, PR_CMD_SITE_ID) == 0 &&
                 (strncasecmp(cmd->argv[1], "CHGRP", 6) == 0 ||
                  strncasecmp(cmd->argv[1], "CHMOD", 6) == 0 ||
                  strncasecmp(cmd->argv[1], "UTIME", 6) == 0)) {
        register unsigned int i;
        char *tmp = "";

        for (i = 3; i <= cmd->argc-1; i++) {
          tmp = pstrcat(cmd->tmp_pool, tmp, *tmp ? " " : "", cmd->argv[i],
            NULL);
        }

        sstrncpy(argp, dir_abs_path(cmd->tmp_pool, tmp, TRUE), sizeof(arg));

      } else {

        /* Some commands (i.e. DELE, MKD, RMD, XMKD, and XRMD) have associated
         * filenames that are not stored in the session.xfer structure; these
         * should be expanded properly as well.
         */
        if (pr_cmd_cmp(cmd, PR_CMD_DELE_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_LIST_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_MDTM_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_MKD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_MLSD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_MLST_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_NLST_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_RMD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_XMKD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_XRMD_ID) == 0) {
          sstrncpy(arg, dir_abs_path(cmd->tmp_pool, cmd->arg, TRUE),
            sizeof(arg));

        } else if (pr_cmd_cmp(cmd, PR_CMD_MFMT_ID) == 0) {
          /* MFMT has, as its filename, the second argument. */
          sstrncpy(arg, dir_abs_path(cmd->tmp_pool, cmd->argv[2], TRUE),
            sizeof(arg));

        } else {
          /* All other situations get a "-".  */
          sstrncpy(argp, "-", sizeof(arg));
        }
      }
      break;

    case 'F':
      argp = arg;

      if (pr_cmd_cmp(cmd, PR_CMD_RNTO_ID) == 0) {
        char *path;

        path = dir_best_path(cmd->tmp_pool,
          pr_fs_decode_path(cmd->tmp_pool, cmd->arg));
        sstrncpy(arg, path, sizeof(arg));

      } else if (session.xfer.p &&
                 session.xfer.path) {
        sstrncpy(argp, session.xfer.path, sizeof(arg));

      } else {
        /* Some commands (i.e. DELE, MKD, RMD, XMKD, and XRMD) have associated
         * filenames that are not stored in the session.xfer structure; these
         * should be expanded
         * properly as well.
         */
        if (pr_cmd_cmp(cmd, PR_CMD_DELE_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_MKD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_RMD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_XMKD_ID) == 0 ||
            pr_cmd_cmp(cmd, PR_CMD_XRMD_ID) == 0) {
          char *path;

          path = dir_best_path(cmd->tmp_pool,
            pr_fs_decode_path(cmd->tmp_pool, cmd->arg));
          sstrncpy(arg, path, sizeof(arg));

        } else {
          sstrncpy(argp, "-", sizeof(arg));
        }
      }
      break;

    case 'g': {
      argp = arg;

      if (session.group != NULL) {
        sstrncpy(argp, session.group, sizeof(arg));

      } else {
        sstrncpy(argp, "-", sizeof(arg));
      }

      break;
    }

    case 'H':
      argp = arg;
      sstrncpy(argp, cmd->server->ServerAddress, sizeof(arg));
      break;

    case 'h':
      argp = arg;
      sstrncpy(argp, pr_netaddr_get_sess_remote_name(), sizeof(arg));
      break;

    case 'I':
      argp = arg;
      snprintf(argp, sizeof(arg), "%" PR_LU, (pr_off_t) session.total_raw_in);
      break;

    case 'J':
      argp = arg;
      if (pr_cmd_cmp(cmd, PR_CMD_PASS_ID) == 0 &&
          session.hide_password) {
        sstrncpy(argp, "(hidden)", sizeof(arg));

      } else {
        sstrncpy(argp, cmd->arg, sizeof(arg));
      }
      break;

    case 'L':
      argp = arg;
      sstrncpy(argp, pr_netaddr_get_ipstr(pr_netaddr_get_sess_local_addr()),
        sizeof(arg));
      break;

    case 'l': {
      char *rfc1413_ident;

      argp = arg;
      rfc1413_ident = pr_table_get(session.notes, "mod_ident.rfc1413-ident",
        NULL);
      if (rfc1413_ident == NULL)
        rfc1413_ident = "UNKNOWN";

      sstrncpy(argp, rfc1413_ident, sizeof(arg));
      break;
    }

    case 'm':
      argp = arg;
      sstrncpy(argp, cmd->argv[0], sizeof(arg));
      break;

    case 'O':
      argp = arg;
      snprintf(argp, sizeof(arg), "%" PR_LU, (pr_off_t) session.total_raw_out);
      break;

    case 'P':
      argp = arg;
      snprintf(argp, sizeof(arg), "%lu", (unsigned long) getpid());
      break;

    case 'p': 
      argp = arg;
      snprintf(argp, sizeof(arg), "%d", main_server->ServerPort);
      break;

    case 'r':
      argp = arg;
      if (pr_cmd_cmp(cmd, PR_CMD_PASS_ID) == 0 &&
          session.hide_password) {
        sstrncpy(argp, C_PASS " (hidden)", sizeof(arg));

      } else {
        sstrncpy(argp, pr_cmd_get_displayable_str(cmd, NULL), sizeof(arg));
      }
      break;

    case 's': {
      char *resp_code = NULL;
      int res;

      argp = arg;

      res = pr_response_get_last(cmd->tmp_pool, &resp_code, NULL);
      if (res == 0 &&
          resp_code != NULL) {
        sstrncpy(argp, resp_code, sizeof(arg));

      } else {
        sstrncpy(argp, "-", sizeof(arg));
      }

      break;
    }

    case 'S': {
      char *resp_msg = NULL;
      int res;

      argp = arg;

      res = pr_response_get_last(cmd->tmp_pool, NULL, &resp_msg);
      if (res == 0 &&
          resp_msg != NULL) {
        sstrncpy(argp, resp_msg, sizeof(arg));

      } else {
        sstrncpy(argp, "-", sizeof(arg));
      }

      break;
    }

    case 'T':
      argp = arg;
      if (session.xfer.p &&
          session.xfer.start_time.tv_sec > 0) {
        struct timeval end_time;
      
        gettimeofday(&end_time, NULL);
        end_time.tv_sec -= session.xfer.start_time.tv_sec;

        if (end_time.tv_usec >= session.xfer.start_time.tv_usec) {
          end_time.tv_usec -= session.xfer.start_time.tv_usec;

        } else {
          end_time.tv_usec = 1000000L - (session.xfer.start_time.tv_usec -
            end_time.tv_usec);
          end_time.tv_sec--;
        }
      
        snprintf(argp, sizeof(arg), "%lu.%03lu",
          (unsigned long) end_time.tv_sec,
          (unsigned long) (end_time.tv_usec / 1000));

      } else {
        sstrncpy(argp, "0.0", sizeof(arg));
      }
      break;

    case 'U': {
      char *login_user;

      argp = arg;

      login_user = pr_table_get(session.notes, "mod_auth.orig-user", NULL);
      if (login_user == NULL) {
        login_user = "-";
      }

      sstrncpy(argp, login_user, sizeof(arg));
      break;
    }

    case 'u': {
      argp = arg;

      if (session.user != NULL) {
        sstrncpy(argp, session.user, sizeof(arg));

      } else {
        sstrncpy(argp, "-", sizeof(arg));
      }

      break;
    }

    case 'V':
      argp = arg;
      sstrncpy(argp, pr_netaddr_get_dnsstr(pr_netaddr_get_sess_local_addr()),
        sizeof(arg));
      break;

    case 'v':
      argp = arg;
      sstrncpy(argp, main_server->ServerName, sizeof(arg));
      break;

    case 'w': {
      char *rnfr_path = "-";

      if (pr_cmd_cmp(cmd, PR_CMD_RNTO_ID) == 0) {
        rnfr_path = pr_table_get(session.notes, "mod_core.rnfr-path", NULL);
        if (rnfr_path != NULL) {
          rnfr_path = dir_abs_path(cmd->tmp_pool,
            pr_fs_decode_path(cmd->tmp_pool, rnfr_path), TRUE);

        } else {
          rnfr_path = "-";
        }
      }
 
      argp = arg; 
      sstrncpy(argp, rnfr_path, sizeof(arg));
      break;
    }

    case '%':
      argp = "%";
      break;

    default:
      argp = "{UNKNOWN TAG}";
      break;
  }

  short_tag = pstrdup(cmd->tmp_pool, argp);
  pr_trace_msg(trace_channel, 15, "returning short tag '%s' for tag '%%%c'",
    short_tag, tag);

  return short_tag;
}

static char *named_query_type(cmd_rec *cmd, char *name) {
  config_rec *c = NULL;
  char *query = NULL;

  query = pstrcat(cmd->tmp_pool, "SQLNamedQuery_", name, NULL);
  c = find_config(main_server->conf, CONF_PARAM, query, FALSE);

  if (c)
    return c->argv[0];

  sql_log(DEBUG_FUNC, "no '%s' SQLNamedQuery found", name);
  errno = ENOENT;
  return NULL;
}

static modret_t *process_named_query(cmd_rec *cmd, char *name, int flags) {
  config_rec *c;
  char *conn_name, *query = NULL, *tmp = NULL, *argp = NULL;
  char outs[SQL_MAX_STMT_LEN+1], *outsp = NULL;
  char *esc_arg = NULL;
  modret_t *mr = NULL;
  int num = 0;

  sql_log(DEBUG_FUNC, ">>> process_named_query '%s'", name);

  /* Check for a query by that name. */
  query = pstrcat(cmd->tmp_pool, "SQLNamedQuery_", name, NULL);

  c = find_config(main_server->conf, CONF_PARAM, query, FALSE);
  if (c) {
    size_t arglen, outs_remain = sizeof(outs)-1;

    conn_name = get_query_named_conn(c);
    set_named_conn_backend(conn_name);

    /* Select string fixup */
    memset(outs, '\0', sizeof(outs));
    outsp = outs;

    for (tmp = c->argv[1]; *tmp; ) {
      char *tag = NULL;

      if (*tmp == '%') {
        if (*(++tmp) == '{') {
          char *tmp_query = NULL;
	  
          if (*tmp != '\0')
            tmp_query = ++tmp;

          /* Find the full tag to use */
          while (*tmp && *tmp != '}')
            tmp++;

          tag = pstrndup(cmd->tmp_pool, tmp_query, (tmp - tmp_query));
          if (tag) {
            register unsigned int i;
            size_t taglen = strlen(tag);
            unsigned char is_numeric_tag = TRUE;

            for (i = 0; i < taglen-1; i++) {
              if (!PR_ISDIGIT(tag[i])) {
                is_numeric_tag = FALSE;
                break;
              }
            }

            if (is_numeric_tag) {
              num = resolve_numeric_tag(cmd, tag);
              if (num < 0) {
                set_named_conn_backend(NULL);
                return PR_ERROR_MSG(cmd, MOD_SQL_VERSION,
                  "out-of-bounds numeric reference in query");
              }

              esc_arg = cmd->argv[num+2];

            } else {
              argp = (char *) resolve_long_tag(cmd, tag);
              if (argp == NULL) {
                set_named_conn_backend(NULL);
                return PR_ERROR_MSG(cmd, MOD_SQL_VERSION,
                  "malformed reference %{?} in query");
              }

              mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, conn_name,
                argp), "sql_escapestring");
              if (check_response(mr, flags) < 0) {
                set_named_conn_backend(NULL);
                return PR_ERROR_MSG(cmd, MOD_SQL_VERSION, "database error");
              }

              esc_arg = (char *) mr->data;
            }

          } else {
            set_named_conn_backend(NULL);
            return PR_ERROR_MSG(cmd, MOD_SQL_VERSION,
              "malformed reference %{?} in query");
          }

        } else {
          argp = resolve_short_tag(cmd, *tmp);

          mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, conn_name,
            argp), "sql_escapestring");
          if (check_response(mr, flags) < 0) {
            set_named_conn_backend(NULL);
            return PR_ERROR_MSG(cmd, MOD_SQL_VERSION, "database error");
          }

          esc_arg = (char *) mr->data;
        }

        arglen = strlen(esc_arg);
        if (outs_remain > arglen) {
          sstrcat(outsp, esc_arg, outs_remain);
          outsp += arglen;
          outs_remain -= arglen;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) for tag (%Lu bytes) when processing named "
            "query '%s', ignoring tag", (unsigned long) outs_remain,
            (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) arglen, name);
        }

        if (*tmp != '\0')
          tmp++;

      } else {
        if (outs_remain > 0) {
          *outsp++ = *tmp;
          outs_remain--;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.  And break out the processing loop.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) for input when processing named query '%s', "
            "skipping", (unsigned long) outs_remain,
            (unsigned long) SQL_MAX_STMT_LEN, name);
          break;
        }

        if (*tmp != '\0')
          tmp++;
      }
    }
      
    *outsp = '\0';

    /* Construct our return data based on the type of query */
    if (strcasecmp(c->argv[0], SQL_UPDATE_C) == 0) {
      query = pstrcat(cmd->tmp_pool, c->argv[2], " SET ", outs, NULL);
      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, conn_name, query), 
        "sql_update");

    } else if (strcasecmp(c->argv[0], SQL_INSERT_C) == 0) {
      query = pstrcat(cmd->tmp_pool, "INTO ", c->argv[2], " VALUES (",
        outs, ")", NULL);
      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, conn_name, query),
        "sql_insert");

    } else if (strcasecmp(c->argv[0], SQL_FREEFORM_C) == 0) {
      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, conn_name, outs),
        "sql_query");

    } else if (strcasecmp(c->argv[0], SQL_SELECT_C) == 0) {
      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, conn_name, outs),
        "sql_select");

    } else {
      mr = PR_ERROR_MSG(cmd, MOD_SQL_VERSION, "unknown NamedQuery type");
    }

  } else {
    mr = PR_ERROR(cmd);
  }
 
  set_named_conn_backend(NULL);

  sql_log(DEBUG_FUNC, "<<< process_named_query '%s'", name);
  return mr;
}

MODRET process_sqllog(cmd_rec *cmd, config_rec *c, const char *label,
    int flags) {
  char *qname = NULL;
  char *type = NULL;
  modret_t *mr = NULL;

  qname = c->argv[0];

  sql_log(DEBUG_FUNC, ">>> %s (%s)", label, c->name);

  type = named_query_type(cmd, qname);
  if (type) {
    if (strcasecmp(type, SQL_UPDATE_C) == 0 ||
        strcasecmp(type, SQL_FREEFORM_C) == 0 ||
        strcasecmp(type, SQL_INSERT_C) == 0) {
      mr = process_named_query(cmd, qname, flags);
      if (check_response(mr, flags) < 0)
        return mr;

    } else {
      sql_log(DEBUG_WARN, "named query '%s' is not an INSERT, UPDATE, or "
        "FREEFORM query", qname);
    }

  } else {
    sql_log(DEBUG_WARN, "named query '%s' cannot be found", qname);
  }

  sql_log(DEBUG_FUNC, "<<< %s (%s)", label, c->name);
  return mr;
}

static int eventlog_master(const char *event_name) {
  char *name = NULL;
  cmd_rec *cmd = NULL;
  config_rec *c = NULL;
  modret_t *mr = NULL;

  if (!(cmap.engine & SQL_ENGINE_FL_LOG)) {
    return 0;
  }
 
  /* Need to create fake cmd_rec for dispatching, since we need
   * cmd->pool, cmd->tmp_pool.  The cmd_rec MUST have
   * fake/unknown name (i.e. cmd->argv[0], cmd->cmd_id), so that it does
   * not run afoul of other logging variables.
   */
  cmd = _sql_make_cmd(session.pool, 1, "EVENT");
 
  name = pstrcat(cmd->tmp_pool, "SQLLog_Event_", event_name, NULL);

  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    int flags = 0;

    pr_signals_handle();

    if (c->argc == 3 &&
        strncmp(c->argv[2], "ignore", 7) == 0) {
      flags |= SQL_LOG_FL_IGNORE_ERRORS;
    }

    pr_trace_msg(trace_channel, 12,
      "executing SQLNamedQuery '%s' for event '%s'", (char *) c->argv[0],
      event_name);
    mr = process_sqllog(cmd, c, "eventlog_master", flags);
    if (mr != NULL &&
        MODRET_ISERROR(mr)) {
      SQL_FREE_CMD(cmd);
      return -1;
    }

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  SQL_FREE_CMD(cmd);
  return 0;
}

MODRET log_master(cmd_rec *cmd) {
  char *name = NULL;
  config_rec *c = NULL;
  modret_t *mr = NULL;

  if (!(cmap.engine & SQL_ENGINE_FL_LOG))
    return PR_DECLINED(cmd);
  
  /* handle explicit queries */
  name = pstrcat(cmd->tmp_pool, "SQLLog_", cmd->argv[0], NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    int flags = 0;

    pr_signals_handle();

    if (c->argc == 2 &&
        strncmp(c->argv[1], "ignore", 7) == 0) {
      flags |= SQL_LOG_FL_IGNORE_ERRORS;
    }

    mr = process_sqllog(cmd, c, "log_master", flags);
    if (mr != NULL &&
        MODRET_ISERROR(mr)) {

      /* We always return DECLINED if we're in the LOG_CMD/LOG_CMD_ERR phase,
       * regardless of the SQLLog processing outcome (Bug#3633).
       */
      if (session.curr_phase == LOG_CMD ||
          session.curr_phase == LOG_CMD_ERR) {
        return PR_DECLINED(cmd);
      }

      return mr;
    }

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }
  
  /* handle implicit queries */
  name = pstrcat(cmd->tmp_pool, "SQLLog_*", NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    int flags = 0;

    pr_signals_handle();

    if (c->argc == 2 &&
        strncmp(c->argv[1], "ignore", 7) == 0) {
      flags |= SQL_LOG_FL_IGNORE_ERRORS;
    }

    mr = process_sqllog(cmd, c, "log_master", flags);
    if (mr != NULL &&
        MODRET_ISERROR(mr)) {

      /* We always return DECLINED if we're in the LOG_CMD/LOG_CMD_ERR phase,
       * regardless of the SQLLog processing outcome (Bug#3633).
       */
      if (session.curr_phase == LOG_CMD ||
          session.curr_phase == LOG_CMD_ERR) {
        return PR_DECLINED(cmd);
      }

      return mr;
    }

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  return PR_DECLINED(cmd);
}

MODRET err_master(cmd_rec *cmd) {
  char *name = NULL;
  config_rec *c = NULL;
  modret_t *mr = NULL;

  if (!(cmap.engine & SQL_ENGINE_FL_LOG))
    return PR_DECLINED(cmd);
  
  /* handle explicit errors */
  name = pstrcat(cmd->tmp_pool, "SQLLog_ERR_", cmd->argv[0], NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    int flags = 0;

    pr_signals_handle();

    if (c->argc == 2 &&
        strncmp(c->argv[1], "ignore", 7) == 0) {
      flags |= SQL_LOG_FL_IGNORE_ERRORS;
    }

    mr = process_sqllog(cmd, c, "err_master", flags);
    if (mr != NULL &&
        MODRET_ISERROR(mr)) {

      /* We always return DECLINED if we're in the LOG_CMD/LOG_CMD_ERR phase,
       * regardless of the SQLLog processing outcome (Bug#3633).
       */
      if (session.curr_phase == LOG_CMD ||
          session.curr_phase == LOG_CMD_ERR) {
        return PR_DECLINED(cmd);
      }

      return mr;
    }

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }
  
  /* handle implicit errors */
  name = pstrcat(cmd->tmp_pool, "SQLLog_ERR_*", NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    int flags = 0;

    pr_signals_handle();

    if (c->argc == 2 &&
        strncmp(c->argv[1], "ignore", 7) == 0) {
      flags |= SQL_LOG_FL_IGNORE_ERRORS;
    }

    mr = process_sqllog(cmd, c, "err_master", flags);
    if (mr != NULL &&
        MODRET_ISERROR(mr)) {

      /* We always return DECLINED if we're in the LOG_CMD/LOG_CMD_ERR phase,
       * regardless of the SQLLog processing outcome (Bug#3633).
       */
      if (session.curr_phase == LOG_CMD ||
          session.curr_phase == LOG_CMD_ERR) {
        return PR_DECLINED(cmd);
      }

      return mr;
    }

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  return PR_DECLINED(cmd);
}

MODRET info_master(cmd_rec *cmd) {
  char *type = NULL;
  char *name = NULL;
  config_rec *c = NULL;
  char outs[SQL_MAX_STMT_LEN+1], *outsp;
  char *argp = NULL; 
  char *tmp = NULL, *resp_code = NULL;
  modret_t *mr = NULL;
  sql_data_t *sd = NULL;

  if (!(cmap.engine & SQL_ENGINE_FL_LOG))
    return PR_DECLINED(cmd);

  /* process explicit handlers */
  name = pstrcat(cmd->tmp_pool, "SQLShowInfo_", cmd->argv[0], NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    size_t arglen = 0, outs_remain = sizeof(outs)-1;

    sql_log(DEBUG_FUNC, ">>> info_master (%s)", name);

    /* we now have at least one config_rec.  Take the output string from 
     * each, and process it -- resolve tags, and when we find a named 
     * query, run it and get info from it. 
     */

    pr_signals_handle();

    memset(outs, '\0', sizeof(outs));
    outsp = outs;

    for (tmp = c->argv[1]; *tmp; ) {
      pr_signals_handle();

      if (*tmp == '%') {
        /* is the tag a named_query reference?  If so, process the 
         * named query, otherwise process it as a normal tag.. 
         */
	  
        if (*(++tmp) == '{') {
	  char *query = NULL;

	  if (*tmp != '\0')
            query = ++tmp;
	    
          /* get the name of the query */
          while (*tmp && *tmp != '}')
            tmp++;
	    
          query = pstrndup(cmd->tmp_pool, query, (tmp - query));

          /* make sure it's a SELECT query */
	    
          type = named_query_type(cmd, query);
          if (type && (strcasecmp(type, SQL_SELECT_C) == 0 ||
                       strcasecmp(type, SQL_FREEFORM_C) == 0)) {
            mr = process_named_query(cmd, query, 0);
            if (check_response(mr, 0) < 0) {
              memset(outs, '\0', sizeof(outs));
              break;

            } else {
              sd = (sql_data_t *) mr->data;
              if (sd->rnum == 0 ||
                  sd->data[0] == NULL) {
                /* If no data was returned, show nothing.  Just continue
                 * on to the next SQLShowInfo.
                 */
                memset(outs, '\0', sizeof(outs));
                break;

              } else {
                if (strncasecmp(sd->data[0], "null", 5) == 0) {
                  /* Treat the text "null" the same as a real null; just
                   * continue on.
                   */
                  memset(outs, '\0', sizeof(outs));
                  break;
                }

                argp = sd->data[0];
              }
            }

          } else {
            /* Can't use an INSERT/UPDATE query for retrieving info.  Skip
             * to the next SQLShowInfo.
             */
            memset(outs, '\0', sizeof(outs));
            break;
          }

        } else {
          argp = resolve_short_tag(cmd, *tmp);
        }

        arglen = strlen(argp);
        if (outs_remain > arglen) {
          sstrcat(outsp, argp, outs_remain);
          outsp += arglen;
          outs_remain -= arglen;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) for tag (%Lu bytes) when processing "
            "SQLShowInfo query '%s', ignoring tag",
            (unsigned long) outs_remain, (unsigned long) SQL_MAX_STMT_LEN,
            (unsigned long) arglen, name);
        }

        if (*tmp != '\0')
          tmp++;

      } else {
        if (outs_remain > 0) {
          *outsp++ = *tmp;
          outs_remain--;

        } else {
          /* Log when this happens; it means we have more input than
           * buffer space.  And break out of the processing loop.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) when processing SQLShowInfo query '%s', "
            "ignoring input", (unsigned long) outs_remain,
            (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) arglen);
          break;
        }

        if (*tmp != '\0')
          tmp++;
      }
    }
      
    *outsp = '\0';

    /* Add the response, if we have one. */
    if (strlen(outs) > 0) {
      /* We keep track of the response code used, as we will need it when
       * flushing the added lines out to the client.
       */
      resp_code = c->argv[0];
      pr_response_add(resp_code, "%s", outs);
    }

    sql_log(DEBUG_FUNC, "<<< info_master (%s)", name);

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  /* process implicit handlers */
  name = pstrdup(cmd->tmp_pool, "SQLShowInfo_*");
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    size_t arglen = 0, outs_remain = sizeof(outs)-1;

    sql_log(DEBUG_FUNC, ">>> info_master (%s)", name);

    /* we now have at least one config_rec.  Take the output string from 
     * each, and process it -- resolve tags, and when we find a named 
     * query, run it and get info from it. 
     */

    pr_signals_handle();

    memset(outs, '\0', sizeof(outs));
    outsp = outs;

    for (tmp = c->argv[1]; *tmp; ) {
      pr_signals_handle();

      if (*tmp == '%') {
        /* is the tag a named_query reference?  If so, process the 
         * named query, otherwise process it as a normal tag.. 
         */
        if (*(++tmp) == '{') {
          char *query = NULL;

          if (*tmp != '\0')
            query = ++tmp;

          /* get the name of the query */
          while (*tmp && *tmp != '}')
            tmp++;

          query = pstrndup(cmd->tmp_pool, query, (tmp - query));

          /* make sure it's a SELECT query */

          type = named_query_type(cmd, query);
          if (type && (strcasecmp(type, SQL_SELECT_C) == 0 ||
                       strcasecmp(type, SQL_FREEFORM_C) == 0)) {
            mr = process_named_query(cmd, query, 0);
            if (check_response(mr, 0) < 0) {
              memset(outs, '\0', sizeof(outs));
              break;

            } else {
              sd = (sql_data_t *) mr->data;
              if (sd->rnum == 0 ||
                  sd->data[0] == NULL) {
                /* If no data was returned, show nothing.  Just continue
                 * on to the next SQLShowInfo.
                 */
                memset(outs, '\0', sizeof(outs));
                break;

              } else {
                if (strncasecmp(sd->data[0], "null", 5) == 0) {
                  /* Treat the text "null" the same as a real null; just
                   * continue on.
                   */
                  memset(outs, '\0', sizeof(outs));
                  break;
                }

                argp = sd->data[0];
              }
            }

          } else {
            /* Can't use an INSERT/UPDATE query for retrieving info.  Skip
             * to the next SQLShowInfo.
             */
            memset(outs, '\0', sizeof(outs));
            break;
          }

        } else {
          argp = resolve_short_tag(cmd, *tmp);
        }

        arglen = strlen(argp);
        if (outs_remain > arglen) {
          sstrcat(outsp, argp, outs_remain);
          outsp += arglen;
          outs_remain -= arglen;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) for tag (%Lu bytes) when processing "
            "SQLShowInfo query '%s', ignoring tag",
            (unsigned long) outs_remain, (unsigned long) SQL_MAX_STMT_LEN,
            (unsigned long) arglen, name);
        }

        if (*tmp != '\0')
          tmp++;

      } else {
        if (outs_remain > 0) {
          *outsp++ = *tmp;
          outs_remain--;

        } else {
          /* Log when this happens; it means we have more input than
           * buffer space.  And break out of the processing loop.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) when processing SQLShowInfo query '%s', "
            "ignoring input", (unsigned long) outs_remain,
            (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) arglen);
          break;
        }

        if (*tmp != '\0')
          tmp++;
      }
    }
      
    *outsp = '\0';

    /* Add the response, if we have one. */
    if (strlen(outs) > 0) {
      /* We keep track of the response code used, as we will need it when
       * flushing the added lines out to the client.
       */
      resp_code = c->argv[0];
      pr_response_add(resp_code, "%s", outs);
    }

    sql_log(DEBUG_FUNC, "<<< info_master (%s)", name);

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  return PR_DECLINED(cmd);
}

MODRET errinfo_master(cmd_rec *cmd) {
  char *type = NULL;
  char *name = NULL;
  config_rec *c = NULL;
  char outs[SQL_MAX_STMT_LEN+1], *outsp = NULL;
  char *argp = NULL; 
  char *tmp = NULL, *resp_code = NULL;
  modret_t *mr = NULL;
  sql_data_t *sd = NULL;

  if (!(cmap.engine & SQL_ENGINE_FL_LOG))
    return PR_DECLINED(cmd);

  /* process explicit handlers */
  name = pstrcat(cmd->tmp_pool, "SQLShowInfo_ERR_", cmd->argv[0], NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    size_t arglen = 0, outs_remain = sizeof(outs)-1;

    sql_log(DEBUG_FUNC, ">>> errinfo_master (%s)", name);

    /* we now have at least one config_rec.  Take the output string from 
     * each, and process it -- resolve tags, and when we find a named 
     * query, run it and get info from it. 
     */

    pr_signals_handle();

    memset(outs, '\0', sizeof(outs));
    outsp = outs;

    pr_trace_msg(trace_channel, 15, "processing SQLShowInfo ERR_%s '%s'",
      cmd->argv[0], cmd->argv[1]);

    for (tmp = c->argv[1]; *tmp; ) {
      pr_signals_handle();

      if (*tmp == '%') {
        /* is the tag a named_query reference?  If so, process the 
         * named query, otherwise process it as a normal tag.. 
         */

        if (*(++tmp) == '{') {
          char *query = NULL;

          if (*tmp != '\0')
            query = ++tmp;
	    
          /* get the name of the query */
	  while (*tmp && *tmp != '}') {
            tmp++;
          }

          query = pstrndup(cmd->tmp_pool, query, (tmp - query));

          /* make sure it's a SELECT query */

          type = named_query_type(cmd, query);
          if (type && (strcasecmp(type, SQL_SELECT_C) == 0 ||
                       strcasecmp(type, SQL_FREEFORM_C) == 0)) {
            mr = process_named_query(cmd, query, 0);
            if (check_response(mr, 0) < 0) {
              memset(outs, '\0', sizeof(outs));
              break;

            } else {
              sd = (sql_data_t *) mr->data;

              pr_trace_msg(trace_channel, 13,
                "SQLShowInfo ERR_%s query '%s' returned row count %lu",
                cmd->argv[0], query, sd->rnum);

              if (sd->rnum == 0 ||
                  sd->data[0] == NULL) {

                /* If no data was returned, show nothing.  Just continue
                 * on to the next SQLShowInfo.
                 */
                memset(outs, '\0', sizeof(outs));
                break;

              } else {
                if (strncasecmp(sd->data[0], "null", 5) == 0) {
                  /* Treat the text "null" the same as a real null; just
                   * continue on.
                   */
                  memset(outs, '\0', sizeof(outs));
                  break;
                }

                argp = sd->data[0];
              }
            }

          } else {
            /* Can't use an INSERT/UPDATE query for retrieving info.  Skip
             * to the next SQLShowInfo.
             */
            memset(outs, '\0', sizeof(outs));
            break;
          }

        } else {
          argp = resolve_short_tag(cmd, *tmp);
        }

        arglen = strlen(argp);
        if (outs_remain > arglen) {
          sstrcat(outsp, argp, outs_remain);
          outsp += arglen;
          outs_remain -= arglen;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) for tag (%Lu bytes) when processing "
            "SQLShowInfo query '%s', ignoring tag",
            (unsigned long) outs_remain, (unsigned long) SQL_MAX_STMT_LEN,
            (unsigned long) arglen, name);
        }

        if (*tmp != '\0')
          tmp++;

      } else {
        if (outs_remain > 0) {
          *outsp++ = *tmp;
          outs_remain--;

        } else {
          /* Log when this happens; it means we have more input than
           * buffer space.  And break out of the processing loop.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) when processing SQLShowInfo query '%s', "
            "ignoring input", (unsigned long) outs_remain,
            (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) arglen);
          break;
        }

        if (*tmp != '\0') {
          tmp++;
        }
      }
    }
      
    *outsp = '\0';

    /* Add the response, if we have one. */
    if (strlen(outs) > 0) {
      /* We keep track of the response code used, as we will need it when
       * flushing the added lines out to the client.
       */
      resp_code = c->argv[0];

      if (*resp_code == '4' ||
          *resp_code == '5') {
        pr_trace_msg(trace_channel, 15,
          "adding error response code %s, msg '%s' for SQLShowInfo ERR_%s",
          resp_code, outs, cmd->argv[0]);

        pr_response_add_err(resp_code, "%s", outs);

      } else {
        pr_trace_msg(trace_channel, 15,
          "adding response code %s, msg '%s' for SQLShowInfo ERR_%s", resp_code,
          outs, cmd->argv[0]);

        pr_response_add(resp_code, "%s", outs);
      }
    }

    sql_log(DEBUG_FUNC, "<<< errinfo_master (%s)", name);

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  /* process implicit handlers */
  name = pstrcat(cmd->tmp_pool, "SQLShowInfo_ERR_*", NULL);
  
  c = find_config(main_server->conf, CONF_PARAM, name, FALSE);
  while (c) {
    size_t arglen = 0, outs_remain = sizeof(outs)-1;

    sql_log(DEBUG_FUNC, ">>> errinfo_master (%s)", name);

    /* we now have at least one config_rec.  Take the output string from 
     * each, and process it -- resolve tags, and when we find a named 
     * query, run it and get info from it. 
     */

    pr_signals_handle();

    memset(outs, '\0', sizeof(outs));
    outsp = outs;

    for (tmp = c->argv[1]; *tmp; ) {
      pr_signals_handle();

      if (*tmp == '%') {
        /* is the tag a named_query reference?  If so, process the 
         * named query, otherwise process it as a normal tag.. 
         */
	  
        if (*(++tmp) == '{') {
          char *query = NULL;

          if (*tmp != '\0')
            query = ++tmp;

          /* get the name of the query */
          while (*tmp && *tmp != '}')
            tmp++;
	    
          query = pstrndup(cmd->tmp_pool, query, (tmp - query));

          /* make sure it's a SELECT query */
	    
          type = named_query_type(cmd, query);
          if (type && (strcasecmp(type, SQL_SELECT_C) == 0 ||
                       strcasecmp(type, SQL_FREEFORM_C) == 0)) {
            mr = process_named_query(cmd, query, 0);
            if (check_response(mr, 0) < 0) {
              memset(outs, '\0', sizeof(outs));
              break;

            } else {
              sd = (sql_data_t *) mr->data;
              if (sd->rnum == 0 ||
                  sd->data[0] == NULL) {
                /* If no data was returned, show nothing.  Just continue
                 * on to the next SQLShowInfo.
                 */
                memset(outs, '\0', sizeof(outs));
                break;

              } else {
                if (strncasecmp(sd->data[0], "null", 5) == 0) {
                  /* Treat the text "null" the same as a real null; just
                   * continue on.
                   */
                  memset(outs, '\0', sizeof(outs));
                  break;
                }

                argp = sd->data[0];
              }
            }

          } else {
            /* Can't use an INSERT/UPDATE query for retrieving info.  Skip
             * to the next SQLShowInfo.
             */
            memset(outs, '\0', sizeof(outs));
            break;
          }

        } else {
          argp = resolve_short_tag(cmd, *tmp);
        }

        arglen = strlen(argp);
        if (outs_remain > arglen) {
          sstrcat(outsp, argp, outs_remain);
          outsp += arglen;
          outs_remain -= arglen;

        } else {
          /* Log when this happens; it means we have more input than buffer
           * space.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) for tag (%Lu bytes) when processing "
            "SQLShowInfo query '%s', ignoring tag",
            (unsigned long) outs_remain, (unsigned long) SQL_MAX_STMT_LEN,
            (unsigned long) arglen, name);
        }

        if (*tmp != '\0')
          tmp++;

      } else {
        if (outs_remain > 0) {
          *outsp++ = *tmp;
          outs_remain--;

        } else {
          /* Log when this happens; it means we have more input than
           * buffer space.  And break out of the processing loop.
           */
          sql_log(DEBUG_FUNC, "insufficient statement buffer size "
            "(%lu of %lu bytes) when processing SQLShowInfo query '%s', "
            "ignoring input", (unsigned long) outs_remain,
            (unsigned long) SQL_MAX_STMT_LEN, (unsigned long) arglen);
          break;
        }

        if (*tmp != '\0') {
          tmp++;
        }
      }
    }
      
    *outsp = '\0';

    /* Add the response, if we have one. */
    if (strlen(outs) > 0) {
      /* We keep track of the response code used, as we will need it when
       * flushing the added lines out to the client.
       */
      resp_code = c->argv[0];

      if (*resp_code == '4' ||
          *resp_code == '5') {
        pr_trace_msg(trace_channel, 15,
          "adding error response code %s, msg '%s' for SQLShowInfo ERR_*",
          resp_code, outs);

        pr_response_add_err(resp_code, "%s", outs);

      } else {
        pr_trace_msg(trace_channel, 15,
          "adding response code %s, msg '%s' for SQLShowInfo ERR_*", resp_code,
          outs);

        pr_response_add(resp_code, "%s", outs);
      }
    }

    sql_log(DEBUG_FUNC, "<<< errinfo_master (%s)", name);

    c = find_config_next(c, c->next, CONF_PARAM, name, FALSE);
  }

  return PR_DECLINED(cmd);
}

MODRET sql_cleanup(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_cleanup");

  res = _sql_dispatch(cmd, "sql_cleanup");
  if (check_response(res, 0) < 0) {
    sql_log(DEBUG_FUNC, "%s", "<<< sql_cleanup");
    return res;
  }

  sql_log(DEBUG_FUNC, "%s", "<<< sql_cleanup");
  return res;
}

MODRET sql_closeconn(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_closeconn");
  res = _sql_dispatch(cmd, "sql_close");
  sql_log(DEBUG_FUNC, "%s", "<<< sql_closeconn");

  return res;
}

MODRET sql_defineconn(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_defineconn");
  res = _sql_dispatch(cmd, "sql_defineconnection");
  sql_log(DEBUG_FUNC, "%s", "<<< sql_defineconn");

  return res;
}

MODRET sql_load_backend(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_load_backend");

  if (cmd->argc == 1) {
    sql_set_backend(cmd->argv[0]);

  } else {
    sql_set_backend(NULL);
  }

  res = mod_create_data(cmd, NULL);

  sql_log(DEBUG_FUNC, "%s", "<<< sql_load_backend");
  return res;
}

MODRET sql_openconn(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_openconn");
  res = _sql_dispatch(cmd, "sql_open");
  sql_log(DEBUG_FUNC, "%s", "<<< sql_openconn");

  return res;
}

MODRET sql_prepare(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_prepare");
  res = _sql_dispatch(cmd, "sql_prepare");
  sql_log(DEBUG_FUNC, "%s", "<<< sql_prepare");

  return res;
}

MODRET sql_select(cmd_rec *cmd) {
  modret_t *res;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_select");
  res = _sql_dispatch(cmd, "sql_select");
  sql_log(DEBUG_FUNC, "%s", "<<< sql_select");

  return res;
}

/* sql_lookup: used by third-party modules to get data via a SQL query.  
 * Third party module must pass a legitimate cmd_rec (including tmp_pool), 
 * and the cmd_rec must have only one argument: the name of a SQLNamedQuery.
 *
 * Returns:
 *
 * DECLINED if mod_sql isn't on
 * ERROR    if named query doesn't exist
 * 
 * SHUTS DOWN if query caused an error
 * 
 * otherwise:
 *
 * array_header * in the data slot with the returned data.  It is up to the
 * calling function to know how many pieces of data to expect, and how to
 * parse them.
 */
MODRET sql_lookup(cmd_rec *cmd) {
  char *type = NULL;
  modret_t *mr = NULL;
  sql_data_t *sd = NULL;
  array_header *ah = NULL;

  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc < 1) {
    return PR_ERROR(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> sql_lookup");

  type = named_query_type(cmd, cmd->argv[1]);
  if (type && (strcasecmp(type, SQL_SELECT_C) == 0 ||
	       strcasecmp(type, SQL_FREEFORM_C) == 0)) {
    mr = process_named_query(cmd, cmd->argv[1], 0);
    
    if (mr != NULL &&
        !MODRET_ISERROR(mr)) {
      register unsigned int i;

      sd = (sql_data_t *) mr->data;

      ah = make_array(session.pool, (sd->rnum * sd->fnum) , sizeof(char *));

      /* The right way to do this is to preserve the abstraction of the array
       * header so things don't blow up when it gets freed.
       */
      for (i = 0; i < (sd->rnum * sd->fnum); i++) {
	*((char **) push_array(ah)) = sd->data[i];
      }

      mr = mod_create_data(cmd, (void *) ah);

    } else {
      /* We have an error.  Log it and die. */
      if (check_response(mr, 0) < 0) {
        sql_log(DEBUG_FUNC, "%s", "<<< sql_lookup");
        return mr;
      }
    }

  } else {
    mr = PR_ERROR(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< sql_lookup");
  return mr;
}

MODRET sql_change(cmd_rec *cmd) {
  char *type = NULL;
  modret_t *mr = NULL;

  if (cmap.engine == 0) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc < 1) {
    return PR_ERROR(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> sql_change");

  type = named_query_type(cmd, cmd->argv[1]);
  if (type && ((!strcasecmp(type, SQL_INSERT_C)) || 
	       (!strcasecmp(type, SQL_UPDATE_C)) ||
	       (!strcasecmp(type, SQL_FREEFORM_C)))) {
    /* fixup the cmd_rec */

    mr = process_named_query(cmd, cmd->argv[1], 0);
    if (check_response(mr, 0) < 0) {
      sql_log(DEBUG_FUNC, "%s", "<<< sql_change");
      return mr;
    }

  } else {
    mr = PR_ERROR(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< sql_change");
  return mr;
}

MODRET sql_escapestr(cmd_rec *cmd) {
  modret_t *mr;

  sql_log(DEBUG_FUNC, "%s", ">>> sql_escapestr");

  mr =_sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
    cmd->argv[0]), "sql_escapestring");
  if (check_response(mr, 0) < 0) {
    sql_log(DEBUG_FUNC, "%s", "<<< sql_escapestr");
    return mr;
  }

  sql_log(DEBUG_FUNC, "%s", "<<< sql_escapestr");
  return mr;
}

/*****************************************************************
 *
 * AUTH COMMAND HANDLERS
 *
 *****************************************************************/

MODRET cmd_setpwent(cmd_rec *cmd) {
  sql_data_t *sd = NULL;
  modret_t *mr = NULL;
  char *where = NULL;
  int i = 0, cnt = 0;

  char *username = NULL;
  char *password = NULL;
  char *shell = NULL;
  char *dir = NULL;
  uid_t uid = 0;
  gid_t gid = 0;
  
  struct passwd lpw;

  if (!SQL_USERSET ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_setpwent");

  /* if we've already filled the passwd cache, just reset the curr_passwd */
  if (cmap.passwd_cache_filled) {
    cmap.curr_passwd = passwd_name_cache->head;
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_setpwent");
    return PR_DECLINED(cmd);
  }

  /* single select or not? */
  if (SQL_FASTUSERS) {
    /* retrieve our list of users */

    if (!cmap.usercustomusersetfast) {
      where = sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL);

      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
        cmap.usrtable, cmap.usrfields, where), "sql_select");
      if (check_response(mr, 0) < 0) {
        return mr;
      }
 
      sd = (sql_data_t *) mr->data;

    } else {
      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
        cmap.usercustomusersetfast));
      if (check_response(mr, 0) < 0) {
        return mr;
      }

      if (MODRET_HASDATA(mr)) {
        array_header *ah = (array_header *) mr->data;
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

        /* Assume the query returned 6 columns per row. */
        sd->fnum = 6;
        sd->rnum = ah->nelts / 6;
        sd->data = (char **) ah->elts;

      } else {
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));
        sd->rnum = 0;
      }
    }
 
    /* Walk through the array, adding users to the cache */
    if (sd != NULL) {
      for (i = 0, cnt = 0; cnt < sd->rnum; cnt++) {
        username = sd->data[i++];

        /* if the username is NULL, skip it */
        if (username == NULL)
          continue;

        password = sd->data[i++];

        uid = cmap.defaultuid;
        if (cmap.uidfield) {
          if (sd->data[i]) {
            uid = atoi(sd->data[i++]);
          } else {
            i++;
          }
        }

        gid = cmap.defaultgid;
        if (cmap.gidfield) {
          if (sd->data[i]) {
            gid = atoi(sd->data[i++]);
          } else {
            i++;
          }
        }

        dir = cmap.defaulthomedir;
        if (sd->data[i]) {
          if (strncmp(sd->data[i], "", 2) == 0 ||
              strncmp(sd->data[i], "NULL", 5) == 0) {
            /* Leave dir pointing to the SQLDefaultHomedir, if any. */
            i++;

          } else {
            dir = sd->data[i++];
          }
        }

        if (cmap.shellfield) {
          shell = sd->data[i++];

        } else {
          shell =  "";
        }

        if (uid < cmap.minuseruid) {
          sql_log(DEBUG_INFO, "user UID %lu below SQLMinUserUID %lu, using "
            "SQLDefaultUID %lu", (unsigned long) uid,
            (unsigned long) cmap.minuseruid, (unsigned long) cmap.defaultuid);
          uid = cmap.defaultuid;
        }
      
        if (gid < cmap.minusergid) {
          sql_log(DEBUG_INFO, "user GID %lu below SQLMinUserGID %lu, using "
            "SQLDefaultGID %lu", (unsigned long) gid,
            (unsigned long) cmap.minusergid, (unsigned long) cmap.defaultgid);
          gid = cmap.defaultgid;
        }

        _sql_addpasswd(cmd, username, password, uid, gid, shell, dir);
      } 
    }

  } else {
    /* Retrieve our list of users */

    if (!cmap.usercustomuserset) {
      where = sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL);

      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
        cmap.usrtable, cmap.usrfield, where), "sql_select");
      if (check_response(mr, 0) < 0) {
        return mr;
      }
 
      sd = (sql_data_t *) mr->data;

    } else {
      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
        cmap.usercustomuserset));
      if (check_response(mr, 0) < 0) {
        return mr;
      }

      if (MODRET_HASDATA(mr)) {
        array_header *ah = (array_header *) mr->data;
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

        /* Assume the query only returned 1 column per row. */
        sd->fnum = 1;
        sd->rnum = ah->nelts;
        sd->data = (char **) ah->elts;
      }
    }

    if (sd != NULL) {
      for (cnt = 0; cnt < sd->rnum; cnt++) {
        username = sd->data[cnt];
      
        /* if the username is NULL for whatever reason, skip it */
        if (username == NULL)
          continue;
      
        /* otherwise, add it to the cache */
        lpw.pw_uid = -1;
        lpw.pw_name = username;
        sql_getpasswd(cmd, &lpw);
      }
    }
  }
  
  cmap.passwd_cache_filled = 1;
  cmap.curr_passwd = passwd_name_cache->head;

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_setpwent");
  return PR_DECLINED(cmd);
}

MODRET cmd_getpwent(cmd_rec *cmd) {
  struct passwd *pw;
  modret_t *mr;

  if (!SQL_USERSET ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getpwent");

  /* make sure our passwd cache is complete  */
  if (!cmap.passwd_cache_filled) {
    mr = cmd_setpwent(cmd);
    if (mr->data == NULL) {
      /* something didn't work in the setpwent call */
      sql_log(DEBUG_FUNC, "%s", "<<< cmd_getpwent");
      return PR_DECLINED(cmd);
    }
  }

  if (cmap.curr_passwd != NULL) {
    pw = (struct passwd *) cmap.curr_passwd->data;
    cmap.curr_passwd = cmap.curr_passwd->list_next;

  } else {
    pw = NULL;
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getpwent");

  if (pw == NULL ||
      pw->pw_uid == (uid_t) -1)
    return PR_DECLINED(cmd);

  return mod_create_data(cmd, (void *) pw);
}

MODRET cmd_endpwent(cmd_rec *cmd) {
  if (!SQL_USERSET ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);
  
  sql_log(DEBUG_FUNC, "%s", ">>> cmd_endpwent");
  
  cmap.curr_passwd = NULL;
  
  sql_log(DEBUG_FUNC, "%s", "<<< cmd_endpwent");
  return PR_DECLINED(cmd);
}

MODRET cmd_setgrent(cmd_rec *cmd) {
  modret_t *mr = NULL;
  sql_data_t *sd = NULL;
  int cnt = 0;
  struct group lgr;
  gid_t gid;
  char *groupname = NULL;
  char *grp_mem = NULL;
  char *where = NULL;
  array_header *ah =NULL;
  char *iterator = NULL;
  char *member = NULL;

  if (!SQL_GROUPSET ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_setgrent");

  /* if we've already filled the passwd group, just reset curr_group */
  if (cmap.group_cache_filled) {
    cmap.curr_group = group_name_cache->head;
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_setgrent");
    return PR_DECLINED(cmd);
  }

  if (SQL_FASTGROUPS) {
    /* retrieve our list of groups */

    if (!cmap.groupcustomgroupsetfast) {
      where = sql_prepare_where(0, cmd, 1, cmap.groupwhere, NULL);

      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 5, MOD_SQL_DEF_CONN_NAME,
        cmap.grptable, cmap.grpfields, where, "1"), "sql_select");
      if (check_response(mr, 0) < 0) {
        return mr;
      }
 
      sd = (sql_data_t *) mr->data;
   
    } else {
      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
        cmap.groupcustomgroupsetfast));
      if (check_response(mr, 0) < 0) {
        return mr;
      }

      if (MODRET_HASDATA(mr)) {
        ah = mr->data;
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

        /* Assume the query returned 3 columns per row. */
        sd->fnum = 3;
        sd->rnum = ah->nelts / 3;
        sd->data = (char **) ah->elts;

      } else {
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));
        sd->rnum = 0;
      }
    }
 
    /* for each group, fill our array header and call _sql_addgroup */

    for (cnt = 0; cnt < sd->rnum; cnt ++) {
      /* if the groupname is NULL for whatever reason, skip the row */
      groupname = sd->data[cnt * 3];
      if (groupname == NULL)
        continue;

      gid = (gid_t) atol(sd->data[(cnt * 3) + 1]);
      grp_mem = sd->data[(cnt * 3) + 2];
      
      ah = make_array(cmd->tmp_pool, 10, sizeof(char *));
      iterator = grp_mem;

      for (member = strsep(&iterator, " ,"); member; member = strsep(&iterator, " ,")) {
	if (*member == '\0') {
          continue;
        }

	*((char **) push_array(ah)) = member;
      }

      _sql_addgroup(cmd, groupname, gid, ah);
    }

  } else {
    /* Retrieve our list of groups. */

    if (!cmap.groupcustomgroupset) {
      where = sql_prepare_where(0, cmd, 1, cmap.groupwhere, NULL);
 
      mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 6, MOD_SQL_DEF_CONN_NAME,
        cmap.grptable, cmap.grpfield, where, NULL, "DISTINCT"), "sql_select");
      if (check_response(mr, 0) < 0) {
        return mr;
      }
 
      sd = (sql_data_t *) mr->data;

    } else {
      mr = sql_lookup(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
        cmap.groupcustomgroupset));
      if (check_response(mr, 0) < 0) {
        return mr;
      }

      if (MODRET_HASDATA(mr)) {
        ah = mr->data;
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));

        /* Assume the query only returned 1 column per row. */
        sd->fnum = 1;
        sd->rnum = ah->nelts;
        sd->data = (char **) ah->elts;

      } else {
        sd = pcalloc(cmd->tmp_pool, sizeof(sql_data_t));
        sd->rnum = 0;
      }
    }
 
    for (cnt = 0; cnt < sd->rnum; cnt++) {
      groupname = sd->data[cnt];
      
      /* if the groupname is NULL for whatever reason, skip it */
      if (groupname == NULL)
        continue;
      
      /* otherwise, add it to the cache */
      lgr.gr_gid = -1;
      lgr.gr_name = groupname;
      
      sql_getgroup(cmd, &lgr);
    }
  }
  
  cmap.group_cache_filled = 1;
  cmap.curr_group = group_name_cache->head;

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_setgrent");
  return PR_DECLINED(cmd);
}

MODRET cmd_getgrent(cmd_rec *cmd) {
  struct group *gr;
  modret_t *mr;

  if (!SQL_GROUPSET ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getgrent");

  /* make sure our group cache is complete  */
  if (!cmap.group_cache_filled) {
    mr = cmd_setgrent(cmd);
    if (mr->data == NULL) {
      /* something didn't work in the setgrent call */
      sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgrent");
      return PR_DECLINED(cmd);
    }
  }

  if (cmap.curr_group != NULL) {
    gr = (struct group *) cmap.curr_group->data;
    cmap.curr_group = cmap.curr_group->list_next;

  } else {
    gr = NULL;
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgrent");

  if (gr == NULL ||
      gr->gr_gid == (gid_t) -1)
    return PR_DECLINED(cmd);

  return mod_create_data(cmd, (void *) gr);
}

MODRET cmd_endgrent(cmd_rec *cmd) {
  if (!SQL_GROUPSET ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_endgrent");

  cmap.curr_group = NULL;

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_endgrent");
  return PR_DECLINED(cmd);
}

MODRET cmd_getpwnam(cmd_rec *cmd) {
  struct passwd *pw;
  struct passwd lpw;

  if (!SQL_USERS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH)) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getpwnam");

  lpw.pw_uid = -1;
  lpw.pw_name = cmd->argv[0];
  pw = sql_getpasswd(cmd, &lpw);

  if (pw == NULL ||
      pw->pw_uid == (uid_t) -1) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_getpwnam");
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getpwnam");
  return mod_create_data(cmd, pw);
}

MODRET cmd_getpwuid(cmd_rec *cmd) {
  struct passwd *pw;
  struct passwd lpw;

  if (!SQL_USERS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH)) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getpwuid");

  lpw.pw_uid = *((uid_t *) cmd->argv[0]);
  lpw.pw_name = NULL;
  pw = sql_getpasswd(cmd, &lpw);

  if (pw == NULL ||
      pw->pw_uid == (uid_t) -1) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_getpwuid");
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getpwuid");
  return mod_create_data(cmd, pw);
}

MODRET cmd_getgrnam(cmd_rec *cmd) {
  struct group *gr;
  struct group lgr;

  if (!SQL_GROUPS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getgrnam");

  lgr.gr_gid = -1;
  lgr.gr_name = cmd->argv[0];
  gr = sql_getgroup(cmd, &lgr);

  if (gr == NULL ||
      gr->gr_gid == (gid_t) -1) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgrnam");
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgrnam");
  return mod_create_data(cmd, gr);
}

MODRET cmd_getgrgid(cmd_rec *cmd) {
  struct group *gr;
  struct group lgr;

  if (!SQL_GROUPS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getgrgid");

  lgr.gr_gid = *((gid_t *) cmd->argv[0]);
  lgr.gr_name = NULL;
  gr = sql_getgroup(cmd, &lgr);

  if (gr == NULL ||
      gr->gr_gid == (gid_t) -1) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgrgid");
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgrgid");
  return mod_create_data(cmd, gr);
}

MODRET cmd_auth(cmd_rec *cmd) {
  char *user = NULL;
  struct passwd lpw, *pw;
  modret_t *mr = NULL;

  if (!SQL_USERS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_auth");

  user = cmd->argv[0];

  /* escape our username */
  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 2, MOD_SQL_DEF_CONN_NAME,
    user), "sql_escapestring");
  if (check_response(mr, 0) < 0) {
    return mr;
  }
 
  user = (char *) mr->data;

  lpw.pw_uid = -1;
  lpw.pw_name = cmd->argv[0];

  if ((pw = sql_getpasswd(cmd, &lpw)) && 
      !pr_auth_check(cmd->tmp_pool, pw->pw_passwd, cmd->argv[0],
        cmd->argv[1])) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_auth");
    session.auth_mech = "mod_sql.c";
    return PR_HANDLED(cmd);

  } else {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_auth");
    return PR_DECLINED(cmd);
  }
}

MODRET cmd_check(cmd_rec *cmd) {
  /* Should we bother to see if the hashed password is what we have in the
   * database? or do we simply assume it is, and ignore the fact that we're
   * being passed the username, too? 
   */

  array_header *ah = cmap.auth_list;
  int success = FALSE;
  modret_t *mr = NULL;

  if (!SQL_USERS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH)) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_check");

  if (cmd->argv[0] == NULL) {
    sql_log(DEBUG_AUTH, "%s", "NULL hashed password");

  } else if (cmd->argv[1] == NULL) {
    sql_log(DEBUG_AUTH, "%s", "NULL user name");

  } else if (cmd->argv[2] == NULL) {
    sql_log(DEBUG_AUTH, "%s", "NULL clear password");

  } else {
    register unsigned int i;
    char *ciphertext = cmd->argv[0];
    char *plaintext = cmd->argv[2];

    if (ah == NULL)
      sql_log(DEBUG_AUTH, "%s", "warning: no SQLAuthTypes configured");

    for (i = 0; ah && i < ah->nelts; i++) {
      struct sql_authtype_handler *sah;

      sah = ((struct sql_authtype_handler **) ah->elts)[i];
      sql_log(DEBUG_AUTH, "checking password using SQLAuthType '%s'",
        sah->name);

      mr = sah->cb(cmd, plaintext, ciphertext);
      if (!MODRET_ISERROR(mr)) {
	sql_log(DEBUG_AUTH, "'%s' SQLAuthType handler reports success",
          sah->name);
	success = 1;
	break;

      } else {
        if (MODRET_HASMSG(mr)) {
          char *err_msg;

          err_msg = MODRET_ERRMSG(mr);
          sql_log(DEBUG_AUTH, "'%s' SQLAuthType handler reports failure: %s",
            sah->name, err_msg);

        } else {
          sql_log(DEBUG_AUTH, "'%s' SQLAuthType handler reports failure",
            sah->name);
        }
      }
    }
  }

  if (success) {
    struct passwd lpw;

    /* This and the associated hack in cmd_uid2name are to support
     * UID reuse in the database -- people (for whatever reason) are
     * reusing UIDs/GIDs multiple times, and the displayed owner in a 
     * LIST or NLST needs to match the current user if possible.  This
     * depends on the fact that if we get success, the user exists in the
     * database (is this always true?).
     */

    lpw.pw_uid = -1;
    lpw.pw_name = cmd->argv[1];
    cmap.authpasswd = sql_getpasswd(cmd, &lpw);

    session.auth_mech = "mod_sql.c";
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_check");
    return PR_HANDLED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_check");
  return PR_DECLINED(cmd);
}

MODRET cmd_uid2name(cmd_rec *cmd) {
  char *uid_name = NULL;
  struct passwd *pw;
  struct passwd lpw;
  char uidstr[MOD_SQL_BUFSIZE] = {'\0'};

  if (!SQL_USERS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_uid2name");

  lpw.pw_uid = *((uid_t *) cmd->argv[0]);
  lpw.pw_name = NULL;

  /* check to see if we're looking up the current user */
  if (cmap.authpasswd &&
      lpw.pw_uid == cmap.authpasswd->pw_uid) {
    sql_log(DEBUG_INFO, "%s", "matched current user");
    pw = cmap.authpasswd;

  } else {
    pw = sql_getpasswd(cmd, &lpw);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_uid2name");

  if (pw == NULL)
    return PR_DECLINED(cmd);

  /* In the case of a lookup of a negatively cached UID, the pw_name
   * member will be NULL, which causes an undesired handling by
   * the core code.  Handle this case separately.
   */
  if (pw->pw_name) {
    uid_name = pw->pw_name;

  } else {
    snprintf(uidstr, MOD_SQL_BUFSIZE, "%lu",
      (unsigned long) *((uid_t *) cmd->argv[0]));
    uid_name = uidstr;
  }

  return mod_create_data(cmd, uid_name);
}

MODRET cmd_gid2name(cmd_rec *cmd) {
  char *gid_name = NULL;
  struct group *gr;
  struct group lgr;
  char gidstr[MOD_SQL_BUFSIZE];

  if (!SQL_GROUPS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_gid2name");

  lgr.gr_gid = *((gid_t *) cmd->argv[0]);
  lgr.gr_name = NULL;
  gr = sql_getgroup(cmd, &lgr);

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_gid2name");

  if (gr == NULL)
    return PR_DECLINED(cmd);

  /* In the case of a lookup of a negatively cached GID, the gr_name
   * member will be NULL, which causes an undesired handling by
   * the core code.  Handle this case separately.
   */
  if (gr->gr_name) {
    gid_name = gr->gr_name;

  } else {
    memset(gidstr, '\0', sizeof(gidstr));
    snprintf(gidstr, sizeof(gidstr)-1, "%lu",
      (unsigned long) *((gid_t *) cmd->argv[0]));
    gid_name = gidstr;
  }

  return mod_create_data(cmd, gid_name);
}

MODRET cmd_name2uid(cmd_rec *cmd) {
  struct passwd *pw;
  struct passwd lpw;

  if (!SQL_USERS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_name2uid");

  lpw.pw_uid = -1;
  lpw.pw_name = cmd->argv[0];

  /* check to see if we're looking up the current user */
  if (cmap.authpasswd && 
      strcmp(lpw.pw_name, cmap.authpasswd->pw_name) == 0) {
    sql_log(DEBUG_INFO, "%s", "matched current user");
    pw = cmap.authpasswd;

  } else {
    pw = sql_getpasswd(cmd, &lpw);
  }

  if (pw == NULL ||
      pw->pw_uid == (uid_t) -1) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_name2uid");
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_name2uid");
  return mod_create_data(cmd, (void *) &pw->pw_uid);
}

MODRET cmd_name2gid(cmd_rec *cmd) {
  struct group *gr;
  struct group lgr;

  if (!SQL_GROUPS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH))
    return PR_DECLINED(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_name2gid");

  lgr.gr_gid = -1;
  lgr.gr_name = cmd->argv[0];
  gr = sql_getgroup(cmd, &lgr);

  if (gr == NULL ||
      gr->gr_gid == (gid_t) -1) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_name2gid");
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_name2gid");
  return mod_create_data(cmd, (void *) &gr->gr_gid);
}

MODRET cmd_getgroups(cmd_rec *cmd) {
  int res;

  if (!SQL_GROUPS ||
      !(cmap.engine & SQL_ENGINE_FL_AUTH)) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getgroups");

  res = sql_getgroups(cmd);
  if (res < 0) {
    sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgroups");
    return PR_DECLINED(cmd); 
  }

  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getgroups");
  return mod_create_data(cmd, (void *) &res);
}

MODRET cmd_getstats(cmd_rec *cmd) {
  modret_t *mr;
  char *query;
  sql_data_t *sd;
  char *usrwhere, *where;

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getstats");

  if (!cmap.sql_fstor) {
    return PR_DECLINED(cmd);
  }

  usrwhere = pstrcat(cmd->tmp_pool, cmap.usrfield, " = '", _sql_realuser(cmd),
    "'", NULL);

  where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, usrwhere,
    sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL), NULL);

  query = pstrcat(cmd->tmp_pool, cmap.sql_fstor, ", ",
		  cmap.sql_fretr, ", ", cmap.sql_bstor, ", ",
		  cmap.sql_bretr, NULL);
  
  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
    cmap.usrtable, query, where), "sql_select");
  if (check_response(mr, 0) < 0) {
    return mr;
  }
 
  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getstats");

  sd = mr->data;

  if (sd->rnum == 0)
    return PR_ERROR(cmd);

  return mod_create_data(cmd, sd->data);
}

MODRET cmd_getratio(cmd_rec *cmd) {
  modret_t *mr;
  char *query;
  sql_data_t *sd;
  char *usrwhere, *where;

  if (!cmap.sql_frate) {
    return PR_DECLINED(cmd);
  }

  sql_log(DEBUG_FUNC, "%s", ">>> cmd_getratio");

  usrwhere = pstrcat(cmd->tmp_pool, cmap.usrfield, " = '", _sql_realuser(cmd),
    "'", NULL);

  where = sql_prepare_where(SQL_PREPARE_WHERE_FL_NO_TAGS, cmd, 2, usrwhere,
    sql_prepare_where(0, cmd, 1, cmap.userwhere, NULL), NULL);

  query = pstrcat(cmd->tmp_pool, cmap.sql_frate, ", ",
		  cmap.sql_fcred, ", ", cmap.sql_brate, ", ",
		  cmap.sql_bcred, NULL);
  
  mr = _sql_dispatch(_sql_make_cmd(cmd->tmp_pool, 4, MOD_SQL_DEF_CONN_NAME,
    cmap.usrtable, query, where), "sql_select");
  if (check_response(mr, 0) < 0)
    return mr;
  
  sql_log(DEBUG_FUNC, "%s", "<<< cmd_getratio");

  sd = mr->data;

  if (sd->rnum == 0)
    return PR_ERROR(cmd);

  return mod_create_data(cmd, sd->data);
}

/*****************************************************************
 *
 * CONFIGURATION DIRECTIVE HANDLERS
 *
 *****************************************************************/

MODRET set_sqlratiostats(cmd_rec * cmd)
{
  int b;

  CHECK_CONF(cmd, CONF_ROOT | CONF_GLOBAL);

  switch (cmd->argc - 1) {
  default:
    CONF_ERROR(cmd, "requires a boolean or 4 field names: "
               "fstor fretr bstor bretr");
  case 1:
    if ((b = get_boolean(cmd, 1)) == -1)
      CONF_ERROR(cmd, "requires a boolean or 4 field names: "
                 "fstor fretr bstor bretr");
    if (b)
      add_config_param_str("SQLRatioStats", 4,
                           "fstor", "fretr", "bstor", "bretr");
    break;

  case 4:
    add_config_param_str("SQLRatioStats", 4,
                         (void *) cmd->argv[1], (void *) cmd->argv[2],
                         (void *) cmd->argv[3], (void *) cmd->argv[4]);
  }

  return PR_HANDLED(cmd);
}

MODRET set_sqlnegativecache(cmd_rec *cmd) {
  int bool = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected a Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(unsigned char));
  *((unsigned char *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* usage: SQLOptions opt1 [opt2 ...] */
MODRET set_sqloptions(cmd_rec *cmd) {
  config_rec *c;
  unsigned long opts = 0UL;
  register unsigned int i;

  if (cmd->argc-1 == 0)
    CONF_ERROR(cmd, "wrong number of parameters");

  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  c = add_config_param(cmd->argv[0], 1, NULL);

  for (i = 1; i < cmd->argc; i++) {
    if (strcasecmp(cmd->argv[i], "NoDisconnectOnError") == 0) {
      opts |= SQL_OPT_NO_DISCONNECT_ON_ERROR;

    } else if (strcasecmp(cmd->argv[i], "UseNormalizedGroupSchema") == 0) {
      opts |= SQL_OPT_USE_NORMALIZED_GROUP_SCHEMA;

    } else if (strcasecmp(cmd->argv[i], "NoReconnect") == 0) {
      opts |= SQL_OPT_NO_RECONNECT;

    } else if (strcasecmp(cmd->argv[i], "IgnoreConfigFile") == 0) {
      opts |= SQL_OPT_IGNORE_CONFIG_FILE;

    } else {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "unknown SQLOption '",
        cmd->argv[i], "'", NULL));
    }
  }

  c->argv[0] = pcalloc(c->pool, sizeof(unsigned long));
  *((unsigned long *) c->argv[0]) = opts;

  return PR_HANDLED(cmd);
}

MODRET set_sqlratios(cmd_rec * cmd)
{
  int b;

  CHECK_CONF(cmd, CONF_ROOT | CONF_GLOBAL);

  switch (cmd->argc - 1) {
  default:
    CONF_ERROR(cmd, "requires a boolean or 4 field names: "
               "frate fcred brate bcred");
  case 1:
    if ((b = get_boolean(cmd, 1)) == -1)
      CONF_ERROR(cmd, "requires a boolean or 4 field names: "
                 "frate fcred brate bcred");
    if (b)
      add_config_param_str("SQLRatios", 4,
                           "frate", "fcred", "brate", "bcred");
    break;

  case 4:
    add_config_param_str("SQLRatios", 4,
                         (void *) cmd->argv[1], (void *) cmd->argv[2],
                         (void *) cmd->argv[3], (void *) cmd->argv[4]);
  }

  return PR_HANDLED(cmd);
}

MODRET add_virtualstr(char *name, cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  add_config_param_str(name, 1, (void *) cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: SQLUserInfo table(s) usernamefield passwdfield UID GID homedir
 *           shell | custom:/<sql-named-query>[/<sql-named-query>[/<sql-named-query>[/<sql-named-query>]]]
 */
MODRET set_sqluserinfo(cmd_rec *cmd) {

  if (cmd->argc-1 != 1 &&
      cmd->argc-1 != 7) {
    CONF_ERROR(cmd, "missing parameters");
  }

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc-1 == 1) {
    char *user = NULL, *userbyid = NULL, *userset = NULL, *usersetfast = NULL;
    char *ptr = NULL;

    /* If only one paramter is used, it must be of the "custom:/" form. */
    if (strncmp("custom:/", cmd->argv[1], 8) != 0) {
      CONF_ERROR(cmd, "badly formatted parameter");
    }

    ptr = strchr(cmd->argv[1] + 8, '/');
    if (ptr == NULL) {
      add_config_param_str("SQLCustomUserInfoByName", 1, cmd->argv[1] + 8);
      return PR_HANDLED(cmd);
    }

    *ptr = '\0';
    user = cmd->argv[1] + 8;
    userbyid = ptr + 1;

    add_config_param_str("SQLCustomUserInfoByName", 1, user);

    ptr = strchr(userbyid, '/');
    if (ptr == NULL) {
      add_config_param_str("SQLCustomUserInfoByID", 1, userbyid);
      return PR_HANDLED(cmd);
    }

    *ptr = '\0';
    userset = ptr + 1;

    add_config_param_str("SQLCustomUserInfoByID", 1, userbyid);

    ptr = strchr(userset, '/');
    if (ptr == NULL) {
      add_config_param_str("SQLCustomUserInfoAllNames", 1, userset);
      return PR_HANDLED(cmd);
    }

    *ptr = '\0';
    usersetfast = ptr + 1;

    add_config_param_str("SQLCustomUserInfoAllNames", 1, userset);
    add_config_param_str("SQLCustomUserInfoAllUsers", 1, usersetfast);
    return PR_HANDLED(cmd);
  }

  /* required to exist - not even going to check them. */
  add_config_param_str("SQLUserTable", 1, (void *) cmd->argv[1]);
  add_config_param_str("SQLUsernameField", 1, (void *) cmd->argv[2]);
  add_config_param_str("SQLPasswordField", 1, (void *) cmd->argv[3]);

  /* These could be "NULL" */
  if (strncasecmp("null", cmd->argv[4], 5) != 0)
    add_config_param_str("SQLUidField", 1, (void *) cmd->argv[4]);

  if (strncasecmp("null", cmd->argv[5], 5) != 0)
    add_config_param_str("SQLGidField", 1, (void *) cmd->argv[5]);

  if (strncasecmp("null", cmd->argv[6], 5) != 0)
    add_config_param_str("SQLHomedirField", 1, (void *) cmd->argv[6]);

  if (strncasecmp("null", cmd->argv[7], 5) != 0)
    add_config_param_str("SQLShellField", 1, (void *) cmd->argv[7]);

  return PR_HANDLED(cmd);
}

/* usage: SQLUserPrimaryKey column-name */
MODRET set_sqluserprimarykey(cmd_rec *cmd) {
  return add_virtualstr(cmd->argv[0], cmd);
}

MODRET set_sqluserwhereclause(cmd_rec *cmd) {
  return add_virtualstr(cmd->argv[0], cmd);
}

/* usage: SQLGroupInfo table(s) groupnamefield gidfield membersfield */
/* usage: SQLGroupInfo table(s) groupnamefield gidfield membersfield |
 *        custom:/<sql-named-query>/<sql-named-query>/sql-named-query[/<sql-named-query[/<sql-named-query>]]
 */
MODRET set_sqlgroupinfo(cmd_rec *cmd) {

  if (cmd->argc-1 != 1 &&
      cmd->argc-1 != 4) {
    CONF_ERROR(cmd, "missing parameters");
  }

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc-1 == 1) {
    char *groupbyname = NULL, *groupbyid = NULL, *groupmembers = NULL,
      *groupset = NULL, *groupsetfast = NULL;
    char *ptr = NULL;

    /* If only one paramter is used, it must be of the "custom:/" form. */
    if (strncmp("custom:/", cmd->argv[1], 8) != 0) {
      CONF_ERROR(cmd, "badly formatted parameter");
    }

    ptr = strchr(cmd->argv[1] + 8, '/');
    if (ptr == NULL) {
      CONF_ERROR(cmd, "badly formatted parameter");
    }

    *ptr = '\0';
    groupbyname = cmd->argv[1] + 8;
    groupbyid = ptr + 1;

    add_config_param_str("SQLCustomGroupInfoByName", 1, groupbyname);

    ptr = strchr(groupbyid, '/');
    if (ptr == NULL) {
      CONF_ERROR(cmd, "badly formatted parameter");
    }

    *ptr = '\0';
    groupmembers = ptr + 1;

    add_config_param_str("SQLCustomGroupInfoByID", 1, groupbyid);

    ptr = strchr(groupmembers, '/');
    if (ptr == NULL) {
      add_config_param_str("SQLCustomGroupInfoMembers", 1, groupmembers);
      return PR_HANDLED(cmd);
    }

    *ptr = '\0';
    groupset = ptr + 1;

    add_config_param_str("SQLCustomGroupInfoMembers", 1, groupmembers);

    ptr = strchr(groupset, '/');
    if (ptr == NULL) {
      add_config_param_str("SQLCustomGroupInfoAllNames", 1, groupset);
      return PR_HANDLED(cmd);
    }

    *ptr = '\0';
    groupsetfast = ptr + 1;

    add_config_param_str("SQLCustomGroupInfoAllNames", 1, groupset);
    add_config_param_str("SQLCustomGroupInfoAllGroups", 1, groupsetfast);
    return PR_HANDLED(cmd);
  }

  /* required to exist - not even going to check them. */
  add_config_param_str("SQLGroupTable", 1, cmd->argv[1]);
  add_config_param_str("SQLGroupnameField", 1, cmd->argv[2]);
  add_config_param_str("SQLGroupGIDField", 1, cmd->argv[3]);
  add_config_param_str("SQLGroupMembersField", 1, cmd->argv[4]);

  return PR_HANDLED(cmd);
}

/* usage: SQLGroupPrimaryKey column-name */
MODRET set_sqlgroupprimarykey(cmd_rec *cmd) {
  return add_virtualstr(cmd->argv[0], cmd);
}

MODRET set_sqlgroupwhereclause(cmd_rec *cmd) {
  return add_virtualstr(cmd->argv[0], cmd);
}

MODRET set_sqldefaulthomedir(cmd_rec *cmd) {
  return add_virtualstr("SQLDefaultHomedir", cmd);
}

/* usage: SQLLog cmdlist query-name ["IGNORE_ERRORS"] */
MODRET set_sqllog(cmd_rec *cmd) {
  config_rec *c;
  char *name, *namep;
  char *cmds;
  char *iterator;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc < 3 ||
      cmd->argc > 4) {
    CONF_ERROR(cmd, "expected cmdlist query-name [IGNORE_ERRORS]");
  }

  /* For each element in the command list, add a 'SQLLog_CMD' config_rec..
   * this is an optimization that speeds up logging and also simplifies the
   * logging code, since there's no need to run through and parse a bunch
   * of potenitally unused SQLLog statements each time any command is run.
   */
  
  cmds = cmd->argv[1];
  iterator = cmds;

  for (name = strsep(&iterator, ", "); name; name = strsep(&iterator, ", ")) {
    if (*name == '\0')
      continue;
    for (namep = name; *namep != '\0'; namep++)
      *namep = toupper(*namep);
    
    name = pstrcat(cmd->tmp_pool, "SQLLog_", name, NULL);
    if (cmd->argc == 4 &&
        strcasecmp(cmd->argv[3], "IGNORE_ERRORS") == 0) {
      c = add_config_param_str(name, 2, cmd->argv[2], "ignore");

    } else {
      c = add_config_param_str(name, 1, cmd->argv[2]);
    }

    if (pr_module_exists("mod_ifsession.c")) {
      /* If the mod_ifsession module is in use, then we need to set the
       * CF_MULTI flag, so that SQLLog directives that appear in
       * mod_ifsession's <IfClass>/<IfGroup>/<IfUser> sections work
       * properly.
       */
      c->flags |= CF_MULTI;
    }
  }
  
  return PR_HANDLED(cmd);
}

/* usage: SQLLogFile path */
MODRET set_sqllogfile(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: SQLLogOnEvent event query-name ["IGNORE_ERRORS"] */
MODRET set_sqllogonevent(cmd_rec *cmd) {
  config_rec *c;
  char *event_name, *config_name;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc < 3 ||
      cmd->argc > 4) {
    CONF_ERROR(cmd, "expected event query-name [IGNORE_ERRORS]");
  }

  /* Provide convenience aliases, falling back to "raw" event name. */ 
  if (strcasecmp(cmd->argv[1], "MaxClientsPerClass") == 0) {
    event_name = "mod_auth.max-clients-per-class";

  } else if (strcasecmp(cmd->argv[1], "MaxClientsPerHost") == 0) {
    event_name = "mod_auth.max-clients-per-host";

  } else if (strcasecmp(cmd->argv[1], "MaxClientsPerUser") == 0) {
    event_name = "mod_auth.max-clients-per-user";

  } else if (strcasecmp(cmd->argv[1], "MaxCommandRate") == 0) {
    event_name = "core.max-command-rate";

  } else if (strcasecmp(cmd->argv[1], "MaxConnectionsPerHost") == 0) {
    event_name = "mod_auth.max-connections-per-host";

  } else if (strcasecmp(cmd->argv[1], "MaxHostsPerUser") == 0) {
    event_name = "mod_auth.max-hosts-per-user";

  } else if (strcasecmp(cmd->argv[1], "MaxLoginAttempts") == 0) {
    event_name = "mod_auth.max-login-attempts";

  } else if (strcasecmp(cmd->argv[1], "RootLogin") == 0) {
    event_name = "mod_auth.root-login";

  } else if (strcasecmp(cmd->argv[1], "TimeoutIdle") == 0) {
    event_name = "core.timeout-idle";

  } else if (strcasecmp(cmd->argv[1], "TimeoutLogin") == 0) {
    event_name = "core.timeout-login";

  } else if (strcasecmp(cmd->argv[1], "TimeoutNoTransfer") == 0) {
    event_name = "core.timeout-no-transfer";

  } else if (strcasecmp(cmd->argv[1], "TimeoutStalled") == 0) {
    event_name = "core.timeout-stalled";

  } else if (strcasecmp(cmd->argv[1], "UserBanned") == 0) {
    event_name = "mod_ban.ban-user";

  } else if (strcasecmp(cmd->argv[1], "HostBanned") == 0) {
    event_name = "mod_ban.ban-host";

  } else if (strcasecmp(cmd->argv[1], "ClassBanned") == 0) {
    event_name = "mod_ban.ban-class";

  } else {
    event_name = cmd->argv[1];
  }

  /* Add a 'SQLLog_Event_<event>' config_rec.  This is an optimization that
   * speeds up logging and also simplifies the logging code.
   */
  config_name = pstrcat(cmd->tmp_pool, "SQLLog_Event_", event_name, NULL);
  if (cmd->argc == 4 &&
      strcasecmp(cmd->argv[3], "IGNORE_ERRORS") == 0) {
    c = add_config_param_str(config_name, 3, cmd->argv[2], event_name,
      "ignore");

  } else {
    c = add_config_param_str(config_name, 2, cmd->argv[2], event_name);
  }

  if (pr_module_exists("mod_ifsession.c")) {
    /* If the mod_ifsession module is in use, then we need to set the
     * CF_MULTI flag, so that SQLLogOnEvent directives that appear in
     * mod_ifsession's <IfClass>/<IfGroup>/<IfUser> sections work
     * properly.
     */
    c->flags |= CF_MULTI;
  }

  /* In addition, we also need to set a SQLLogOnEvent config_rec, for
   * lookup in sess_init(), so that we know to which events to subscribe.
   */
  add_config_param_str(cmd->argv[0], 1, event_name);

  return PR_HANDLED(cmd);
}

/* usage: SQLNamedConnectInfo name backend info [user [pass [ttl]]] */
MODRET set_sqlnamedconnectinfo(cmd_rec *cmd) {
  char *conn_name = NULL;
  char *backend = NULL;
  char *info = NULL;
  char *user = "";
  char *pass = "";
  char *ttl = NULL;
  struct sql_backend *sb;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc-1 < 3 ||
      cmd->argc-1 > 6) {
    CONF_ERROR(cmd, "requires 3 to 6 arguments. Check the mod_sql docs.");
  }

  conn_name = cmd->argv[1];

  backend = cmd->argv[2];
  sb = sql_get_backend(backend);
  if (sb == NULL) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, ": SQLBackend '", backend,
      "' not supported", NULL));
  }

  if (cmd->argc >= 4)
    info = cmd->argv[3];

  if (cmd->argc >= 5)
    user = cmd->argv[4];

  if (cmd->argc >= 6)
    pass = cmd->argv[5];

  /* Note: The only connection policy which is honored for NamedConnInfos
   * is the TTL policy, i.e. for setting a timer on this connect.  Other
   * policies are only honored if set on the default connection via
   * SQLConnectInfo.
   */

  if (cmd->argc >= 7) {
    ttl = cmd->argv[6];

  } else {
    ttl = "0";
  }

  (void) add_config_param_str(cmd->argv[0], 6, conn_name, backend, info, user,
    pass, ttl);

  return PR_HANDLED(cmd);
}

/* usage: SQLNamedQuery name type query-string [table-name] [conn-name] */
MODRET set_sqlnamedquery(cmd_rec *cmd) {
  config_rec *c = NULL;
  char *name = NULL;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc < 4) {
    CONF_ERROR(cmd, "requires at least 3 arguments");
  }

  name = pstrcat(cmd->tmp_pool, "SQLNamedQuery_", cmd->argv[1], NULL);

  if (strcasecmp(cmd->argv[2], "SELECT") == 0) {
    char *conn_name;

    conn_name = MOD_SQL_DEF_CONN_NAME;
    if (cmd->argc == 5) {
        conn_name = cmd->argv[4];
    }

    c = add_config_param_str(name, 3, SQL_SELECT_C, cmd->argv[3], conn_name);

  } else if (strcasecmp(cmd->argv[2], "FREEFORM") == 0) {
    char *conn_name;

    conn_name = MOD_SQL_DEF_CONN_NAME;
    if (cmd->argc == 5) {
        conn_name = cmd->argv[4];
    }

    c = add_config_param_str(name, 3, SQL_FREEFORM_C, cmd->argv[3], conn_name);

  } else if (strcasecmp(cmd->argv[2], "INSERT") == 0) {
    char *conn_name;

    if (cmd->argc < 5) {
      CONF_ERROR(cmd, "expected 'INSERT' query-string table-name");
    }

    conn_name = MOD_SQL_DEF_CONN_NAME;
    if (cmd->argc == 6) {
        conn_name = cmd->argv[5];
    }

    c = add_config_param_str(name, 4, SQL_INSERT_C, cmd->argv[3],
      cmd->argv[4], conn_name);

  } else if (strcasecmp(cmd->argv[2], "UPDATE") == 0) {
    char *conn_name;

    if (cmd->argc < 5) {
      CONF_ERROR(cmd, "expected 'UPDATE' query-string table-name");
    }

    conn_name = MOD_SQL_DEF_CONN_NAME;
    if (cmd->argc == 6) {
        conn_name = cmd->argv[5];
    }

    c = add_config_param_str(name, 4, SQL_UPDATE_C, cmd->argv[3],
      cmd->argv[4], conn_name);

  } else {
    CONF_ERROR(cmd, "type must be SELECT, INSERT, UPDATE, or FREEFORM");
  }

  c->flags |= CF_MULTI;

  return PR_HANDLED(cmd);
}

/* usage: SQLShowInfo cmdlist numeric format-string */
MODRET set_sqlshowinfo(cmd_rec *cmd) {
  config_rec *c = NULL;
  char *iterator = NULL;
  char *namep = NULL;
  char *name = NULL;
  char *cmds = NULL;

  CHECK_ARGS(cmd, 3);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  cmds = pstrdup(cmd->tmp_pool, cmd->argv[1]);
  iterator = cmds;

  for (name = strsep(&iterator, ", "); name; name = strsep(&iterator, ", ")) {
    if (*name == '\0')
      continue;
    for (namep = name; *namep != '\0'; namep++)
      *namep = toupper(*namep);
    
    name = pstrcat(cmd->tmp_pool, "SQLShowInfo_", name, NULL);
    
    c = add_config_param_str(name, 2, cmd->argv[2], cmd->argv[3]);

    if (pr_module_exists("mod_ifsession.c")) {
      /* If the mod_ifsession module is in use, then we need to set the
       * CF_MULTI flag, so that SQLShowInfo directives that appear in
       * mod_ifsession's <IfClass>/<IfGroup>/<IfUser> sections work
       * properly.
       */
      c->flags |= CF_MULTI;
    }
  }

  return PR_HANDLED(cmd);
}

MODRET set_sqlauthenticate(cmd_rec *cmd) {
  config_rec *c = NULL;
  char *arg = NULL;
  int authmask = 0;
  int cnt = 0;

  int groupset_flag = 0;
  int userset_flag = 0;
  int groups_flag = 0;
  int users_flag = 0;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc < 2 ||
      cmd->argc > 5)
    CONF_ERROR(cmd, "requires 1 to 4 arguments. Check the mod_sql docs");

  /* We're setting our authmask here -- we have a bunch of checks needed to
   * make sure users aren't trying to screw around with us.
   */

  if (cmd->argc == 2 &&
      strcasecmp(cmd->argv[1], "on") == 0) {
    authmask = SQL_AUTH_GROUPSET|SQL_AUTH_USERSET|SQL_AUTH_USERS|
      SQL_AUTH_GROUPS;

  } else if (!((cmd->argc == 2) && !strcasecmp(cmd->argv[1], "off"))) {
    for (cnt = 1; cnt < cmd->argc; cnt++) {
      arg = cmd->argv[cnt];
      
      if (strncasecmp("groupset", arg, 8) == 0) {
        if (groupset_flag)
          CONF_ERROR(cmd, "groupset already set");

        if (strcasecmp("groupsetfast", arg) == 0) {
          authmask |= SQL_FAST_GROUPSET;

        } else if (strlen(arg) > 8) {
          CONF_ERROR(cmd, "unknown argument");
        }

        authmask |= SQL_AUTH_GROUPSET;
        groupset_flag = 1;

      } else if (strncasecmp("userset", arg, 7) == 0) {
        if (userset_flag)
          CONF_ERROR(cmd, "userset already set");

        if (strcasecmp("usersetfast", arg) == 0) {
          authmask |= SQL_FAST_USERSET;

        } else if (strlen(arg) > 7) {
          CONF_ERROR(cmd, "unknown argument");
        }

        authmask |= SQL_AUTH_USERSET;
        userset_flag = 1;

      } else if (strncasecmp("groups", arg, 6) == 0) {
        if (groups_flag)
          CONF_ERROR(cmd, "groups already set");
	
        if (strcasecmp("groups*", arg) == 0) {
          pr_log_debug(DEBUG1, "%s: use of '*' in SQLAuthenticate has been deprecated.  Use AuthOrder for setting authoritativeness", cmd->argv[0]);

        } else if (strlen(arg) > 6) {
          CONF_ERROR(cmd, "unknown argument");
        }

        authmask |= SQL_AUTH_GROUPS;
        groups_flag = 1;

      } else if (strncasecmp("users", arg, 5) == 0) {
        if (users_flag)
          CONF_ERROR(cmd, "users already set");

        if (strcasecmp("users*", arg) == 0) {
          pr_log_debug(DEBUG1, "%s: use of '*' in SQLAuthenticate has been deprecated.  Use AuthOrder for setting authoritativeness", cmd->argv[0]);

        } else if (strlen(arg) > 5) {
          CONF_ERROR(cmd, "unknown argument");
        }

        authmask |= SQL_AUTH_USERS;
        users_flag = 1;

      } else {
        CONF_ERROR(cmd, "unknown argument");
      }
    } 
  }
  
  /* Finally, fixup if we've received groupset with no groups,
   * or userset with no users
   */
  if ((groupset_flag && !groups_flag) ||
      (userset_flag && !users_flag)) {
    CONF_ERROR(cmd, "groupset and userset have no meaning without "
      "a corresponding groups or users argument.");
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = authmask;

  return PR_HANDLED(cmd);
}

/* logging stuff */

static char *sql_logfile = NULL;
static int sql_logfd = -1;

static int sql_closelog(void) {

  /* sanity check */
  if (sql_logfd != -1) {
    close(sql_logfd);
    sql_logfd = -1;
    sql_logfile = NULL;
  }

  return 0;
}

int sql_log(int level, const char *fmt, ...) {
  va_list msg;
  int res;

  /* sanity check */
  if (!sql_logfile)
    return 0;

  va_start(msg, fmt);
  res = pr_log_vwritefile(sql_logfd, MOD_SQL_VERSION, fmt, msg);
  va_end(msg);

  return res;
}

static int sql_openlog(void) {
  int res = 0, xerrno = 0;

  /* Sanity checks */
  sql_logfile = get_param_ptr(main_server->conf, "SQLLogFile", FALSE);
  if (sql_logfile == NULL)
    return 0;

  if (strcasecmp(sql_logfile, "none") == 0) {
    sql_logfile = NULL;
    return 0;
  }

  pr_signals_block();
  PRIVS_ROOT
  res = pr_log_openfile(sql_logfile, &sql_logfd, PR_LOG_SYSTEM_MODE);
  xerrno = errno;
  PRIVS_RELINQUISH
  pr_signals_unblock();

  errno = xerrno;
  return res;
}

/* usage: SQLConnectInfo info [user [pass [policy]]] */
MODRET set_sqlconnectinfo(cmd_rec *cmd) {
  char *info = NULL;
  char *user = "";
  char *pass = "";
  char *ttl = NULL;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  if (cmd->argc < 2 ||
      cmd->argc > 5) {
    CONF_ERROR(cmd, "requires 1 to 4 arguments.  Check the mod_sql docs");
  }

  if (cmd->argc > 1)
    info = cmd->argv[1];

  if (cmd->argc > 2)
    user = cmd->argv[2];

  if (cmd->argc > 3)
    pass = cmd->argv[3];

  if (cmd->argc > 4) {
    ttl = cmd->argv[4];

  } else {
    ttl = "0";
  }

  (void) add_config_param_str(cmd->argv[0], 4, info, user, pass, ttl);

  return PR_HANDLED(cmd);
}

/* usage: SQLEngine on|off|auth|log */
MODRET set_sqlengine(cmd_rec *cmd) {
  config_rec *c;
  int engine = -1;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL|CONF_ANON);

  engine = get_boolean(cmd, 1);
  if (engine == -1) {
    /* The parameter is not a boolean; check for "auth" or "log". */
    if (strcasecmp(cmd->argv[1], "auth") == 0) {
      engine = SQL_ENGINE_FL_AUTH;

    } else if (strcasecmp(cmd->argv[1], "log") == 0) {
      engine = SQL_ENGINE_FL_LOG;

    } else {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "unknown SQLEngine parameter '",
        cmd->argv[1], "'", NULL));
    }

  } else {
    if (engine == 1) {
      /* Convert an "on" into a auth|log combination. */
      engine = SQL_ENGINE_FL_AUTH|SQL_ENGINE_FL_LOG;
    }
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = engine;
  c->flags |= CF_MERGEDOWN;

  return PR_HANDLED(cmd);
}

MODRET set_sqlauthtypes(cmd_rec *cmd) {
  array_header *auth_list;
  register unsigned int i;

  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  /* Need *at least* one handler. */
  if (cmd->argc < 2) {
    CONF_ERROR(cmd, "expected at least one SQLAuthType");
  }

  auth_list = make_array(permanent_pool, cmd->argc-1,
    sizeof(struct sql_authtype_handler *));

  /* Walk through our cmd->argv. */
  for (i = 1; i < cmd->argc; i++) {
    struct sql_authtype_handler *sah;

    sah = sql_get_authtype(cmd->argv[i]);
    if (sah == NULL) {
      sql_log(DEBUG_WARN, "unknown SQLAuthType '%s'", cmd->argv[i]);
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "unknown SQLAuthType '",
        cmd->argv[i], "'", NULL));
    }
 
    *((struct sql_authtype_handler **) push_array(auth_list)) = sah;
  }

  (void) add_config_param(cmd->argv[0], 1, auth_list);
  return PR_HANDLED(cmd);
}

/* usage: SQLBackend name */
MODRET set_sqlbackend(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

MODRET set_sqlminid(cmd_rec *cmd) {
  config_rec *c;
  unsigned long val;
  char *endptr = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  val = strtoul(cmd->argv[1], &endptr, 10);

  if (*endptr != '\0')
    CONF_ERROR(cmd, "requires a numeric argument");

  /* Whee! need to check if in the legal range for uid_t and gid_t. */
  if (val == ULONG_MAX &&
      errno == ERANGE) {
    CONF_ERROR(cmd, "the value given is outside the legal range");
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(unsigned long));
  *((unsigned long *) c->argv[0]) = val;

  return PR_HANDLED(cmd);
}

MODRET set_sqlminuseruid(cmd_rec *cmd) {
  config_rec *c = NULL;
  unsigned long val;
  char *endptr = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  val = strtoul(cmd->argv[1], &endptr, 10);

  if (*endptr != '\0')
    CONF_ERROR(cmd, "requires a numeric argument");

  /* Whee! need to check if in the legal range for uid_t. */
  if (val == ULONG_MAX &&
      errno == ERANGE) {
    CONF_ERROR(cmd, "the value given is outside the legal range");
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(uid_t));
  *((uid_t *) c->argv[0]) = val;

  return PR_HANDLED(cmd);
}

MODRET set_sqlminusergid(cmd_rec *cmd) {
  config_rec *c = NULL;
  unsigned long val;
  char *endptr = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  val = strtoul(cmd->argv[1], &endptr, 10);

  if (*endptr != '\0')
    CONF_ERROR(cmd, "requires a numeric argument");

  /* Whee! need to check if in the legal range for gid_t. */
  if (val == ULONG_MAX &&
      errno == ERANGE) {
    CONF_ERROR(cmd, "the value given is outside the legal range");
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(gid_t));
  *((gid_t *) c->argv[0]) = val;

  return PR_HANDLED(cmd);
}

MODRET set_sqldefaultuid(cmd_rec *cmd) {
  int xerrno;
  config_rec *c;
  uid_t val;
  char *endptr = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  errno = 0;
  val = strtoul(cmd->argv[1], &endptr, 10);
  xerrno = errno;

  if (*endptr != '\0')
    CONF_ERROR(cmd, "requires a numeric argument");

  /* Whee! need to check is in the legal range for uid_t. */
  if (xerrno == ERANGE) {
    CONF_ERROR(cmd, "the value given is outside the legal range");
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(uid_t));
  *((uid_t *) c->argv[0]) = val;

  return PR_HANDLED(cmd);
}

MODRET set_sqldefaultgid(cmd_rec *cmd) {
  int xerrno;
  config_rec *c;
  gid_t val;
  char *endptr = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_GLOBAL|CONF_VIRTUAL);

  errno = 0;
  val = strtoul(cmd->argv[1], &endptr, 10);
  xerrno = errno;

  if (*endptr != '\0')
    CONF_ERROR(cmd, "requires a numeric argument");

  /* Whee! need to check is in the legal range for gid_t. */
  if (xerrno == ERANGE) {
    CONF_ERROR(cmd, "the value given is outside the legal range");
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(gid_t));
  *((gid_t *) c->argv[0]) = val;

  return PR_HANDLED(cmd);
}

/* Event handlers
 */

static void sql_chroot_ev(const void *event_data, void *user_data) {
  /* Loop through our list of named connections, making sure that any
   * with a connection policy of PERSESSION are opened.
   */
  if (sql_named_conns != NULL) {
    pool *tmp_pool;
    struct sql_named_conn *snc;

    tmp_pool = make_sub_pool(session.pool);

    for (snc = sql_named_conns; snc; snc = snc->next) {
      pr_signals_handle();

      if (snc->conn_policy == SQL_CONN_POLICY_PERSESSION) {
        cmd_rec *cmd;
        modret_t *mr; 

        cmd = _sql_make_cmd(tmp_pool, 1, snc->conn_name);
        mr = _sql_dispatch(cmd, "sql_open");
        (void) check_response(mr, 0);
        SQL_FREE_CMD(cmd);
      }
    }

    destroy_pool(tmp_pool);
  }
}

static void sql_exit_ev(const void *event_data, void *user_data) {
  config_rec *c;
  cmd_rec *cmd;
  modret_t *mr;

  if (cmap.engine == 0) {
    return;
  }

  /* handle EXIT queries */
  c = find_config(main_server->conf, CONF_PARAM, "SQLLog_EXIT", FALSE);

  while (c) {
    pr_signals_handle();

    /* Since we're exiting the process here (or soon, anyway), we can
     * get away with using the config_rec's pool.
     */
    cmd = _sql_make_cmd(c->pool, 1, "EXIT");

    /* Ignore errors; we're exiting anyway. */
    (void) process_sqllog(cmd, c, "exit_listener", SQL_LOG_FL_IGNORE_ERRORS);

    c = find_config_next(c, c->next, CONF_PARAM, "SQLLog_EXIT", FALSE);
  }

  cmd = _sql_make_cmd(session.pool, 0);
  mr = _sql_dispatch(cmd, "sql_exit");
  (void) check_response(mr, SQL_LOG_FL_IGNORE_ERRORS);

  sql_closelog();
  return;
}

#if defined(PR_SHARED_MODULE)
static void sql_mod_unload_ev(const void *event_data, void *user_data) {
  if (strcmp("mod_sql.c", (const char *) event_data) == 0) {
    destroy_pool(sql_pool);
    sql_pool = NULL;
    sql_backends = NULL;
    sql_auth_list = NULL;

    pr_event_unregister(&sql_module, NULL, NULL);

    (void) sql_unregister_authtype("Backend");
    (void) sql_unregister_authtype("Crypt");
    (void) sql_unregister_authtype("Empty");
    (void) sql_unregister_authtype("Plaintext");

#if defined(HAVE_OPENSSL) || defined(PR_USE_OPENSSL)
    (void) sql_unregister_authtype("OpenSSL");
#endif /* HAVE_OPENSSL */

    close(sql_logfd);
    sql_logfd = -1;
  }
}

#else

static void sql_preparse_ev(const void *event_data, void *user_data) {
  /* If no backends have been registered, croak. */
  if (sql_nbackends == 0) {
    pr_log_pri(PR_LOG_NOTICE, MOD_SQL_VERSION
      ": notice: no backend modules have been registered");
    exit(1);
  }
}
#endif /* PR_SHARED_MODULE */

static void sql_eventlog_ev(const void *event_data, void *user_data) {
  const char *event_name;
  int res;

  event_name = user_data;
  res = eventlog_master(event_name);
  if (res < 0) {
    sql_log(DEBUG_FUNC, "SQLLogOnEvent '%s' query failed", event_name);
  }
}

/* Initialization routines
 */

static int sql_init(void) {
#if defined(PR_SHARED_MODULE)
  pr_event_register(&sql_module, "core.module-unload", sql_mod_unload_ev, NULL);
#else
  pr_event_register(&sql_module, "core.preparse", sql_preparse_ev, NULL);
#endif /* PR_SHARED_MODULE */

  /* Register our built-in auth handlers. */
  (void) sql_register_authtype("Backend", sql_auth_backend);
  (void) sql_register_authtype("Crypt", sql_auth_crypt);
  (void) sql_register_authtype("Empty", sql_auth_empty);
  (void) sql_register_authtype("Plaintext", sql_auth_plaintext);

#if defined(HAVE_OPENSSL) || defined(PR_USE_OPENSSL)
  (void) sql_register_authtype("OpenSSL", sql_auth_openssl);
#endif /* HAVE_OPENSSL */

  return 0;
}

static int sql_sess_init(void) {
  char *authstr = NULL;
  config_rec *c = NULL;
  void *ptr = NULL;
  unsigned char *negative_cache = NULL;
  cmd_rec *cmd = NULL;
  modret_t *mr = NULL;
  sql_data_t *sd = NULL;
  int engine = 0, res = 0;
  char *fieldset = NULL;
  pool *tmp_pool = NULL;

  /* Build a temporary pool */
  tmp_pool = make_sub_pool(session.pool);

  /* Open any configured SQLLogFile */
  res = sql_openlog();
  if (res < 0) {
    if (res == -1) {
      pr_log_pri(PR_LOG_NOTICE, "notice: unable to open SQLLogFile: %s",
        strerror(errno));

    } else if (res == PR_LOG_WRITABLE_DIR) {
      pr_log_pri(PR_LOG_WARNING, "notice: unable to open SQLLogFile: "
          "parent directory is world-writable");

    } else if (res == PR_LOG_SYMLINK) {
      pr_log_pri(PR_LOG_WARNING, "notice: unable to open SQLLogFile: "
          "cannot log to a symbolic link");
    }
  }

  ptr = get_param_ptr(main_server->conf, "SQLBackend", FALSE);
  sql_default_cmdtable = sql_set_backend(ptr);
  if (sql_default_cmdtable == NULL) {
    if (ptr != NULL) {
      sql_log(DEBUG_INFO, "unable to load '%s' SQL backend: %s",
        (char *) ptr, strerror(errno));

    } else {
      sql_log(DEBUG_INFO, "unable to load SQL backend: %s", strerror(errno));
    }

    destroy_pool(tmp_pool);
    return -1;
  }

  /* Construct our internal cache structure for this session. */
  memset(&cmap, 0, sizeof(cmap));

  c = find_config(main_server->conf, CONF_PARAM, "SQLEngine", FALSE);
  if (c != NULL) {
    cmap.engine = engine = *((int *) c->argv[0]);

  } else {
    cmap.engine = engine = (SQL_ENGINE_FL_AUTH|SQL_ENGINE_FL_LOG);
  }

  /* Get our backend info and toss it up */
  cmd = _sql_make_cmd(tmp_pool, 1, "foo");
  mr = _sql_dispatch(cmd, "sql_identify");
  if (check_response(mr, 0) < 0) {
    destroy_pool(tmp_pool);
    return -1;
  }

  sd = (sql_data_t *) mr->data;

  sql_log(DEBUG_INFO, "backend module '%s'", sd->data[0]);
  sql_log(DEBUG_INFO, "backend api    '%s'", sd->data[1]);

  SQL_FREE_CMD(cmd);

  sql_log(DEBUG_FUNC, "%s", ">>> sql_sess_init");

  if (!sql_pool) {
    sql_pool = make_sub_pool(session.pool);
    pr_pool_tag(sql_pool, MOD_SQL_VERSION);
  }

  group_name_cache = make_cache(sql_pool, _group_name, _groupcmp);
  passwd_name_cache = make_cache(sql_pool, _passwd_name, _passwdcmp);
  group_gid_cache = make_cache(sql_pool, _group_gid, _groupcmp);
  passwd_uid_cache = make_cache(sql_pool, _passwd_uid, _passwdcmp);

  cmap.group_cache_filled = 0;
  cmap.passwd_cache_filled = 0;

  cmap.curr_group = NULL;
  cmap.curr_passwd = NULL;

  ptr = get_param_ptr(main_server->conf, "SQLAuthenticate", FALSE);
  if (ptr != NULL) {
    cmap.authmask = *((int *) ptr);

  } else {
    cmap.authmask = SQL_AUTH_GROUPS|SQL_AUTH_USERS|SQL_AUTH_GROUPSET|
      SQL_AUTH_USERSET;
  }

  negative_cache = get_param_ptr(main_server->conf, "SQLNegativeCache",
    FALSE);
  cmap.negative_cache = negative_cache ? *negative_cache : FALSE;

  cmap.defaulthomedir = get_param_ptr(main_server->conf, "SQLDefaultHomedir",
    FALSE);

  pr_sql_opts = 0UL;
  c = find_config(main_server->conf, CONF_PARAM, "SQLOptions", FALSE);
  while (c != NULL) {
    unsigned long opts;

    pr_signals_handle();

    opts = *((unsigned long *) c->argv[0]);
    pr_sql_opts |= opts;

    c = find_config_next(c, c->next, CONF_PARAM, "SQLOptions", FALSE);
  }
 
  ptr = get_param_ptr(main_server->conf, "SQLUserTable", FALSE);
  
  /* if we have no SQLUserTable, SQLUserInfo was not used -- default all */
  
  if (ptr == NULL) {
    cmap.usrtable = MOD_SQL_DEF_USERTABLE;
    cmap.usrfield = MOD_SQL_DEF_USERNAMEFIELD;
    cmap.pwdfield = MOD_SQL_DEF_USERPASSWORDFIELD;
    cmap.uidfield = MOD_SQL_DEF_USERUIDFIELD;
    cmap.gidfield = MOD_SQL_DEF_USERGIDFIELD;
    cmap.homedirfield = MOD_SQL_DEF_USERHOMEDIRFIELD;
    cmap.shellfield = MOD_SQL_DEF_USERSHELLFIELD;

    /* It's possible that custom UserInfo queries were configured.  Check for
     * them.
     */
    ptr = get_param_ptr(main_server->conf, "SQLCustomUserInfoByName", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.usercustom = ptr;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomUserInfoByID", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.usercustombyid = ptr;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomUserInfoAllNames", FALSE);
    if (ptr) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.usercustomuserset = ptr;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomUserInfoAllUsers", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.usercustomusersetfast = ptr;
      }
    }

    if (cmap.usercustomuserset &&
        cmap.usercustomusersetfast &&
        strcmp(cmap.usercustomuserset, cmap.usercustomusersetfast) == 0) {
      sql_log(DEBUG_INFO, "warning: 'userset' and 'usersetfast' custom "
        "SQLUserInfo SQLNamedQuery are the same query ('%s'), probable "
        "misconfiguration", cmap.usercustomuserset);
    }

  } else {
    cmap.usrtable = ptr;
    cmap.usrfield = get_param_ptr(main_server->conf, "SQLUsernameField", FALSE);
    cmap.pwdfield = get_param_ptr(main_server->conf, "SQLPasswordField", FALSE);
    cmap.uidfield = get_param_ptr(main_server->conf, "SQLUidField", FALSE);
    cmap.gidfield = get_param_ptr(main_server->conf, "SQLGidField", FALSE);
    cmap.homedirfield = get_param_ptr(main_server->conf, "SQLHomedirField",
      FALSE);
    cmap.shellfield = get_param_ptr(main_server->conf, "SQLShellField", FALSE);
  }

  /* Build the userfieldset */
  fieldset = pstrcat(tmp_pool, cmap.usrfield, ", ", cmap.pwdfield, NULL);

  if (cmap.uidfield)
    fieldset = pstrcat(tmp_pool, fieldset, ", ", cmap.uidfield, NULL);

  if (cmap.gidfield)
    fieldset = pstrcat(tmp_pool, fieldset, ", ", cmap.gidfield, NULL);

  if (cmap.homedirfield)
    fieldset = pstrcat(tmp_pool, fieldset, ", ", cmap.homedirfield, NULL);

  if (cmap.shellfield)
    fieldset = pstrcat(tmp_pool, fieldset, ", ", cmap.shellfield, NULL);

  cmap.usrfields = pstrdup(sql_pool, fieldset);

  ptr = get_param_ptr(main_server->conf, "SQLGroupTable", FALSE);
  
  /* If we have no ptr, SQLGroupInfo was not used - default all */
  if (ptr == NULL) {
    cmap.grptable = MOD_SQL_DEF_GROUPTABLE;
    cmap.grpfield = MOD_SQL_DEF_GROUPNAMEFIELD;
    cmap.grpgidfield = MOD_SQL_DEF_GROUPGIDFIELD;
    cmap.grpmembersfield = MOD_SQL_DEF_GROUPMEMBERSFIELD;

    /* Check for any configured custom SQLGroupInfo queries. */

    ptr = get_param_ptr(main_server->conf, "SQLCustomGroupInfoByID", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.groupcustombyid = ptr;
        cmap.grpgidfield = NULL;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomGroupInfoByName", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.groupcustombyname = ptr;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomGroupInfoMembers", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.groupcustommembers = ptr;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomGroupInfoAllNames", FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.groupcustomgroupset = ptr;
      }
    }

    ptr = get_param_ptr(main_server->conf, "SQLCustomGroupInfoAllGroups",
      FALSE);
    if (ptr != NULL) {
      config_rec *custom_c = NULL;
      char *named_query = pstrcat(tmp_pool, "SQLNamedQuery_", ptr, NULL);

      custom_c = find_config(main_server->conf, CONF_PARAM, named_query, FALSE);
      if (custom_c == NULL) {
        sql_log(DEBUG_INFO, "error: unable to resolve custom "
          "SQLNamedQuery name '%s'", (char *) ptr);

      } else {
        cmap.groupcustomgroupsetfast = ptr;
      }
    }


  } else {
    cmap.grptable = get_param_ptr(main_server->conf, "SQLGroupTable", FALSE);
    cmap.grpfield = get_param_ptr(main_server->conf, "SQLGroupnameField",
      FALSE);
    cmap.grpgidfield = get_param_ptr(main_server->conf, "SQLGroupGIDField",
      FALSE);
    cmap.grpmembersfield = get_param_ptr(main_server->conf,
      "SQLGroupMembersField", FALSE);
  }

  /* Build the groupfieldset */
  fieldset = pstrcat(tmp_pool, cmap.grpfield, ", ", cmap.grpgidfield, ", ",
    cmap.grpmembersfield, NULL);
  cmap.grpfields = pstrdup(sql_pool, fieldset);

  ptr = get_param_ptr(main_server->conf, "SQLUserWhereClause", FALSE);
  cmap.userwhere = ptr ? pstrcat(sql_pool, "(", ptr, ")", NULL) : NULL;

  ptr = get_param_ptr(main_server->conf, "SQLGroupWhereClause", FALSE);
  cmap.groupwhere = ptr ? pstrcat(sql_pool, "(", ptr, ")", NULL) : NULL;

  ptr = get_param_ptr(main_server->conf, "SQLAuthTypes", FALSE);
  cmap.auth_list = ptr;

  if (cmap.auth_list == NULL &&
      cmap.authmask != 0 &&
      (engine > 0 && engine != SQL_ENGINE_FL_LOG)) {
    sql_log(DEBUG_INFO, "%s", "error: no SQLAuthTypes configured");
  }

  ptr = get_param_ptr(main_server->conf, "SQLMinID", FALSE);
  if (ptr != NULL) {
    cmap.minuseruid = cmap.minusergid = *((unsigned long *) ptr);

  } else {
    ptr = get_param_ptr(main_server->conf, "SQLMinUserUID", FALSE);
    cmap.minuseruid = ptr ? *((uid_t *) ptr) : MOD_SQL_MIN_USER_UID;

    ptr = get_param_ptr(main_server->conf, "SQLMinUserGID", FALSE);
    cmap.minusergid = ptr ? *((gid_t *) ptr) : MOD_SQL_MIN_USER_GID;
  }

  ptr = get_param_ptr(main_server->conf, "SQLDefaultUID", FALSE);
  cmap.defaultuid = ptr ? *((uid_t *) ptr) : MOD_SQL_DEF_UID;

  ptr = get_param_ptr(main_server->conf, "SQLDefaultGID", FALSE);
  cmap.defaultgid = ptr ? *((gid_t *) ptr) : MOD_SQL_DEF_GID;

  c = find_config(main_server->conf, CONF_PARAM, "SQLRatioStats", FALSE);
  if (c) {
    cmap.sql_fstor = c->argv[0];
    cmap.sql_fretr = c->argv[1];
    cmap.sql_bstor = c->argv[2];
    cmap.sql_bretr = c->argv[3];
  }

  c = find_config(main_server->conf, CONF_PARAM, "SQLRatios", FALSE);
  if (c) {
    if (!cmap.sql_fstor) {
      pr_log_pri(PR_LOG_WARNING,
        MOD_SQL_VERSION ": warning: SQLRatios directive ineffective "
        "without SQLRatioStats on");
      sql_log(DEBUG_WARN, "%s", "warning: SQLRatios directive ineffective "
        "without SQLRatioStats on");
    }

    cmap.sql_frate = c->argv[0];
    cmap.sql_fcred = c->argv[1];
    cmap.sql_brate = c->argv[2];
    cmap.sql_bcred = c->argv[3];
  }

  if (!cmap.homedirfield &&
      !cmap.defaulthomedir) {
    cmap.authmask ^= SQL_AUTH_USERS;

    pr_log_pri(PR_LOG_WARNING, MOD_SQL_VERSION
      ": warning: no homedir field and no default specified. "
      "User authentication is OFF");
    sql_log(DEBUG_WARN, "%s",
      "warning: no homedir field and no default specified. "
      "User authentication is OFF");
  }

  c = find_config(main_server->conf, CONF_PARAM, "SQLConnectInfo", FALSE);
  if (c == NULL) {
    cmap.authmask = 0;
    cmap.engine = 0;
    cmap.sql_fstor = NULL;
    cmap.sql_frate = NULL;
    sql_log(DEBUG_WARN, "%s",
      "warning: no SQLConnectInfo specified. mod_sql is OFF");

  } else {
    unsigned int default_conn_policy;

    pr_sql_conn_policy = SQL_CONN_POLICY_PERSESSION;

    if (strcasecmp(c->argv[3], "perconn") == 0 ||
        strcasecmp(c->argv[3], "perconnection") == 0) {
      pr_sql_conn_policy = SQL_CONN_POLICY_PERCONN;

    } else if (strcasecmp(c->argv[3], "percall") == 0) {
      pr_sql_conn_policy = SQL_CONN_POLICY_PERCALL;
    }

    if (sql_define_conn(tmp_pool, MOD_SQL_DEF_CONN_NAME, c->argv[1], c->argv[2],
      c->argv[0], c->argv[3]) < 0) {
      return -1;
    }

    /* Since sql_define_conn() can change the policy (depending on the
     * backend's handling of the connection parameters, we record the
     * default connection policy here.
     */
    default_conn_policy = pr_sql_conn_policy;

    sql_log(DEBUG_INFO, "%s", "connection 'default' successfully established");

    c = find_config(main_server->conf, CONF_PARAM, "SQLNamedConnectInfo",
      FALSE);
    while (c) {
      struct sql_named_conn *snc;
      char *conn_name;

      pr_signals_handle();

      conn_name = c->argv[0];

      /* Avoid duplicated connection names. */
      if (get_named_conn_backend(conn_name) == NULL) {
        pr_sql_conn_policy = SQL_CONN_POLICY_PERSESSION;

        if (strncasecmp(c->argv[5], "perconn", 8) == 0 ||
            strncasecmp(c->argv[5], "perconnection", 14) == 0) {
          pr_sql_conn_policy = SQL_CONN_POLICY_PERCONN;

        } else if (strncasecmp(c->argv[5], "percall", 8) == 0) {
          pr_sql_conn_policy = SQL_CONN_POLICY_PERCALL;
        }

        if (sql_define_conn(tmp_pool, c->argv[0], c->argv[3], c->argv[4],
            c->argv[2], c->argv[5]) < 0) {
          /* Restore the default connection policy. */
          pr_sql_conn_policy = default_conn_policy;

          destroy_pool(tmp_pool);
          return -1;
        }

        /* Add the mapping of the connection name to the backend to the
         * lookup list.
         */
        snc = pcalloc(sql_pool, sizeof(struct sql_named_conn));
        snc->conn_name = conn_name;
        snc->conn_policy = pr_sql_conn_policy;
        snc->backend = c->argv[1];

        if (sql_named_conns != NULL) {
          sql_named_conns->prev = snc;
          snc->next = sql_named_conns;
        }

        sql_named_conns = snc;

        /* Restore the default connection policy. */
        pr_sql_conn_policy = default_conn_policy;

        sql_log(DEBUG_INFO, "connection '%s' successfully established",
          conn_name );

      } else {
        sql_log(DEBUG_INFO, MOD_SQL_VERSION
          ": unable to open SQLNamedConnectInfo '%s': another connection "
          "with that name already exists", conn_name);
      }

      c = find_config_next(c, c->next, CONF_PARAM, "SQLNamedConnectInfo",
        FALSE);
    }
  }

  c = find_config(main_server->conf, CONF_PARAM, "SQLLogOnEvent", FALSE);
  while (c) {
    char *event_name;

    pr_signals_handle();

    event_name = c->argv[0];

    pr_event_register(&sql_module, event_name, sql_eventlog_ev, event_name);
    c = find_config_next(c, c->next, CONF_PARAM, "SQLLogOnEvent", FALSE);
  }

  if (cmap.engine == 0) {
    sql_log(DEBUG_INFO, "mod_sql engine     : off");

  } else if (cmap.engine == (SQL_ENGINE_FL_AUTH|SQL_ENGINE_FL_LOG)) {
    sql_log(DEBUG_INFO, "mod_sql engine     : on");

  } else if (cmap.engine == SQL_ENGINE_FL_AUTH) {
    sql_log(DEBUG_INFO, "mod_sql engine     : auth");

  } else if (cmap.engine == SQL_ENGINE_FL_LOG) {
    sql_log(DEBUG_INFO, "mod_sql engine     : log");
  }

  sql_log(DEBUG_INFO, "negative_cache     : %s", cmap.negative_cache ? "on" : "off");

  authstr = "";

  if (SQL_USERS)
    authstr = pstrcat(tmp_pool, authstr, "users ", NULL);

  if (SQL_GROUPS)
    authstr = pstrcat(tmp_pool, authstr, "groups ", NULL);

  if (SQL_USERSET) {
    if (SQL_FASTUSERS) {
      authstr = pstrcat(tmp_pool, authstr, "userset(fast) ", NULL);

    } else {
      authstr = pstrcat(tmp_pool, authstr, "userset ", NULL);
    }
  }

  if (SQL_GROUPSET) {
    if (SQL_FASTGROUPS) {
      authstr = pstrcat(tmp_pool, authstr, "groupset(fast)", NULL);

    } else {
      authstr = pstrcat(tmp_pool, authstr, "groupset", NULL);
    }
  }

  sql_log(DEBUG_INFO, "authenticate       : %s",
    (!authstr || *authstr=='\0') ? "off" : authstr);

  if (SQL_USERS ||
      cmap.sql_fstor ||
      cmap.sql_frate) {
    sql_log(DEBUG_INFO, "usertable          : %s", cmap.usrtable);
    sql_log(DEBUG_INFO, "userid field       : %s", cmap.usrfield);
  }

  if (SQL_USERS) {
    sql_log(DEBUG_INFO, "password field     : %s", cmap.pwdfield);

    sql_log(DEBUG_INFO, "UID field          : %s",
      (cmap.uidfield ? cmap.uidfield : "NULL"));

    sql_log(DEBUG_INFO, "GID field          : %s",
      (cmap.gidfield ? cmap.gidfield : "NULL"));

    if (cmap.homedirfield)
      sql_log(DEBUG_INFO, "homedir field      : %s", cmap.homedirfield);

    if (cmap.defaulthomedir)
      sql_log(DEBUG_INFO, "homedir(default)   : '%s'", cmap.defaulthomedir);

    sql_log(DEBUG_INFO, "shell field        : %s",
      (cmap.shellfield ? cmap.shellfield : "NULL"));
  }

  if (SQL_GROUPS) {
    sql_log(DEBUG_INFO, "group table        : %s", cmap.grptable);
    sql_log(DEBUG_INFO, "groupname field    : %s", cmap.grpfield);
    sql_log(DEBUG_INFO, "grp GID field      : %s", cmap.grpgidfield);
    sql_log(DEBUG_INFO, "grp members field  : %s", cmap.grpmembersfield);
  }

  if (SQL_USERS) {
    sql_log(DEBUG_INFO, "SQLMinUserUID      : %u", cmap.minuseruid);
    sql_log(DEBUG_INFO, "SQLMinUserGID      : %u", cmap.minusergid);
  }
   
  if (SQL_GROUPS) {
    sql_log(DEBUG_INFO, "SQLDefaultUID      : %u", cmap.defaultuid);
    sql_log(DEBUG_INFO, "SQLDefaultGID      : %u", cmap.defaultgid);
  }

  if (cmap.sql_fstor) {
    sql_log(DEBUG_INFO, "sql_fstor          : %s", cmap.sql_fstor);
    sql_log(DEBUG_INFO, "sql_fretr          : %s", cmap.sql_fretr);
    sql_log(DEBUG_INFO, "sql_bstor          : %s", cmap.sql_bstor);
    sql_log(DEBUG_INFO, "sql_bretr          : %s", cmap.sql_bretr);
  }

  if (cmap.sql_frate) {
    sql_log(DEBUG_INFO, "sql_frate          : %s", cmap.sql_frate);
    sql_log(DEBUG_INFO, "sql_fcred          : %s", cmap.sql_fcred);
    sql_log(DEBUG_INFO, "sql_brate          : %s", cmap.sql_brate);
    sql_log(DEBUG_INFO, "sql_bcred          : %s", cmap.sql_bcred);
  }

  sql_log(DEBUG_FUNC, "%s", "<<< sql_sess_init");

  destroy_pool(tmp_pool);

  pr_event_register(&sql_module, "core.chroot", sql_chroot_ev, NULL);
  pr_event_register(&sql_module, "core.exit", sql_exit_ev, NULL);

  return 0;
}

/*****************************************************************
 *
 * HANDLER TABLES
 *
 *****************************************************************/

static conftable sql_conftab[] = {
  { "SQLConnectInfo",	 set_sqlconnectinfo,	NULL },
  { "SQLNamedConnectInfo",set_sqlnamedconnectinfo, NULL },

  { "SQLAuthenticate",	set_sqlauthenticate,	NULL },
  { "SQLAuthTypes",	set_sqlauthtypes,	NULL },
  { "SQLBackend",	set_sqlbackend,		NULL },
  { "SQLEngine",	set_sqlengine,		NULL },
  { "SQLOptions",	set_sqloptions,		NULL },

  { "SQLUserInfo", set_sqluserinfo, NULL},
  { "SQLUserPrimaryKey", set_sqluserprimarykey, NULL },
  { "SQLUserWhereClause", set_sqluserwhereclause, NULL },

  { "SQLGroupInfo", set_sqlgroupinfo, NULL },
  { "SQLGroupPrimaryKey", set_sqlgroupprimarykey, NULL },
  { "SQLGroupWhereClause", set_sqlgroupwhereclause, NULL },

  { "SQLMinID", set_sqlminid, NULL },
  { "SQLMinUserUID", set_sqlminuseruid, NULL },
  { "SQLMinUserGID", set_sqlminusergid, NULL },
  { "SQLDefaultUID", set_sqldefaultuid, NULL },
  { "SQLDefaultGID", set_sqldefaultgid, NULL },

  { "SQLNegativeCache", set_sqlnegativecache, NULL },

  { "SQLRatios", set_sqlratios, NULL },
  { "SQLRatioStats", set_sqlratiostats, NULL },

  { "SQLDefaultHomedir", set_sqldefaulthomedir, NULL },

  { "SQLLog", set_sqllog, NULL },
  { "SQLLogFile", set_sqllogfile, NULL },
  { "SQLLogOnEvent", set_sqllogonevent, NULL },
  { "SQLNamedQuery", set_sqlnamedquery, NULL },
  { "SQLShowInfo", set_sqlshowinfo, NULL },

  { NULL, NULL, NULL }
};

static cmdtable sql_cmdtab[] = {
  { PRE_CMD,		C_PASS,	G_NONE, sql_pre_pass,	FALSE, 	FALSE },
  { POST_CMD,		C_PASS,	G_NONE, sql_post_pass,	FALSE, 	FALSE },
  { PRE_CMD,		C_DELE,	G_NONE, sql_pre_dele,	FALSE,	FALSE },
  { POST_CMD,		C_RETR,	G_NONE,	sql_post_retr,	FALSE,	FALSE },
  { POST_CMD,		C_STOR,	G_NONE,	sql_post_stor,	FALSE,	FALSE },
  { POST_CMD,		C_ANY,	G_NONE,	info_master,	FALSE,	FALSE },
  { POST_CMD_ERR,	C_ANY,	G_NONE,	errinfo_master,	FALSE,	FALSE },
  { LOG_CMD,		C_ANY,	G_NONE,	log_master,	FALSE,	FALSE },
  { LOG_CMD_ERR,	C_ANY,	G_NONE,	err_master,	FALSE,	FALSE },

  /* Module hooks */
  { HOOK,	"sql_change",		G_NONE,	sql_change,	FALSE, FALSE }, 
  { HOOK,	"sql_cleanup",		G_NONE, sql_cleanup,	FALSE, FALSE },
  { HOOK,	"sql_close_conn",	G_NONE, sql_closeconn,	FALSE, FALSE },
  { HOOK,	"sql_define_conn",	G_NONE, sql_defineconn,	FALSE, FALSE },
  { HOOK,	"sql_escapestr",	G_NONE,	sql_escapestr,	FALSE, FALSE },
  { HOOK,	"sql_load_backend",	G_NONE,	sql_load_backend,FALSE, FALSE },
  { HOOK,	"sql_lookup",		G_NONE,	sql_lookup,	FALSE, FALSE },
  { HOOK,	"sql_open_conn",	G_NONE,	sql_openconn,	FALSE, FALSE },
  { HOOK,	"sql_prepare",		G_NONE, sql_prepare,	FALSE, FALSE },
  { HOOK,	"sql_select",		G_NONE, sql_select,	FALSE, FALSE },

  { 0, NULL }
};

static authtable sql_authtab[] = {
  { 0, "setpwent",	cmd_setpwent },
  { 0, "getpwent",	cmd_getpwent },
  { 0, "endpwent",	cmd_endpwent },
  { 0, "setgrent",	cmd_setgrent },
  { 0, "getgrent",	cmd_getgrent },
  { 0, "endgrent",	cmd_endgrent },
  { 0, "getpwnam",	cmd_getpwnam },
  { 0, "getpwuid",	cmd_getpwuid },
  { 0, "getgrnam",	cmd_getgrnam },
  { 0, "getgrgid",	cmd_getgrgid },
  { 0, "auth",		cmd_auth     },
  { 0, "check",		cmd_check    },
  { 0, "uid2name",	cmd_uid2name },
  { 0, "gid2name",	cmd_gid2name },
  { 0, "name2uid",	cmd_name2uid },
  { 0, "name2gid",	cmd_name2gid },
  { 0, "getgroups",	cmd_getgroups },

  /* Note: these should be HOOKs, and in the cmdtab. */
  { 0, "getstats",	cmd_getstats },
  { 0, "getratio",	cmd_getratio },

  { 0, NULL, NULL }
};

module sql_module = {

  /* Always NULL */
  NULL, NULL,

  /* Module API version */
  0x20,

  /* Module name */
  "sql",

  /* Module configuration directive table */
  sql_conftab,

  /* Module command handler table */
  sql_cmdtab,

  /* Module auth handler table */
  sql_authtab,

  /* Module initialization */
  sql_init,

  /* Session initialization */
  sql_sess_init,

  /* Module version */
  MOD_SQL_VERSION
};

