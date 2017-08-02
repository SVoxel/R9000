/*
 * ProFTPD: mod_site_misc -- a module implementing miscellaneous SITE commands
 *
 * Copyright (c) 2004-2011 The ProFTPD Project
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
 * As a special exemption, The ProFTPD Project team and other respective
 * copyright holders give permission to link this program with OpenSSL, and
 * distribute the resulting executable, without including the source code for
 * OpenSSL in the source distribution.
 *
 * $Id: mod_site_misc.c,v 1.21 2011-12-11 02:33:14 castaglia Exp $
 */

#include "conf.h"

#define MOD_SITE_MISC_VERSION		"mod_site_misc/1.5"

extern pr_response_t *resp_list, *resp_err_list;

static unsigned int site_misc_engine = TRUE;

static int site_misc_check_filters(cmd_rec *cmd, const char *path) {
#ifdef PR_USE_REGEX
  pr_regex_t *pre = get_param_ptr(CURRENT_CONF, "PathAllowFilter", FALSE);
  if (pre != NULL &&
      pr_regexp_exec(pre, path, 0, NULL, 0, 0, 0) != 0) {
    pr_log_debug(DEBUG2, MOD_SITE_MISC_VERSION
      ": 'SITE %s' denied by PathAllowFilter", cmd->arg);
    return -1;
  }

  pre = get_param_ptr(CURRENT_CONF, "PathDenyFilter", FALSE);
  if (pre != NULL &&
      pr_regexp_exec(pre, path, 0, NULL, 0, 0, 0) == 0) {
    pr_log_debug(DEBUG2, MOD_SITE_MISC_VERSION
      ": 'SITE %s' denied by PathDenyFilter", cmd->arg);
    return -1;
  }
#endif

  return 0;
}

static int site_misc_create_dir(const char *dir) {
  struct stat st;
  int res;

  pr_fs_clear_cache();

  res = pr_fsio_stat(dir, &st);
  if (res < 0 &&
      errno != ENOENT) {
    int xerrno = errno;

    pr_log_debug(DEBUG2, MOD_SITE_MISC_VERSION ": error checking '%s': %s",
      dir, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  if (res == 0) {
    /* Directory already exists */
    return 1;
  }

  if (pr_fsio_mkdir(dir, 0777) < 0) {
    int xerrno = errno;

    pr_log_debug(DEBUG2, MOD_SITE_MISC_VERSION ": error creating '%s': %s",
      dir, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  return 0;
}

static int site_misc_create_path(pool *p, const char *path) {
  struct stat st;
  char *curr_path, *tmp_path;

  pr_fs_clear_cache();

  if (pr_fsio_stat(path, &st) == 0)
    return 0;

  /* The given path should already be canonicalized; we do not need to worry
   * if it is relative to the current working directory or not.
   */

  tmp_path = pstrdup(p, path);

  curr_path = "/";
  while (tmp_path &&
         *tmp_path) {
    char *curr_dir;
    int res;
    cmd_rec *cmd;
    pool *sub_pool;

    pr_signals_handle();

    curr_dir = strsep(&tmp_path, "/");
    curr_path = pdircat(p, curr_path, curr_dir, NULL);

    /* Dispatch the fake C_MKD command, e.g. for mod_quotatab. */
    sub_pool = pr_pool_create_sz(p, 64);
    cmd = pr_cmd_alloc(sub_pool, 2, pstrdup(sub_pool, C_MKD),
      pstrdup(sub_pool, curr_path));
    cmd->arg = pstrdup(cmd->pool, curr_path);
    cmd->cmd_class = CL_DIRS|CL_WRITE;

    res = pr_cmd_dispatch_phase(cmd, PRE_CMD, 0);
    if (res < 0) {
      int xerrno = errno;

      pr_log_debug(DEBUG3, MOD_SITE_MISC_VERSION
        ": creating directory '%s' blocked by MKD handler: %s", curr_path,
        strerror(xerrno));

      pr_cmd_dispatch_phase(cmd, POST_CMD_ERR, 0);
      pr_cmd_dispatch_phase(cmd, LOG_CMD_ERR, 0);
      pr_response_clear(&resp_err_list);

      destroy_pool(sub_pool);
      sub_pool = NULL;
      cmd = NULL;

      errno = xerrno;
      return -1;
    }

    res = site_misc_create_dir(curr_path);
    if (res < 0) {
      int xerrno = errno;

      pr_cmd_dispatch_phase(cmd, POST_CMD_ERR, 0);
      pr_cmd_dispatch_phase(cmd, LOG_CMD_ERR, 0);
      pr_response_clear(&resp_err_list);

      destroy_pool(sub_pool);
      sub_pool = NULL;
      cmd = NULL;

      errno = xerrno;
      return -1;
    }

    pr_cmd_dispatch_phase(cmd, POST_CMD, 0);
    pr_cmd_dispatch_phase(cmd, LOG_CMD, 0);
    pr_response_clear(&resp_list);

    destroy_pool(sub_pool);
    sub_pool = NULL;
    cmd = NULL;
  }
 
  return 0;
}

static int site_misc_delete_dir(pool *p, const char *dir) {
  void *dirh;
  struct dirent *dent;
  int res;
  cmd_rec *cmd;
  pool *sub_pool;

  dirh = pr_fsio_opendir(dir);
  if (dirh == NULL)
    return -1;

  while ((dent = pr_fsio_readdir(dirh)) != NULL) {
    struct stat st;
    char *file;

    pr_signals_handle();

    if (strncmp(dent->d_name, ".", 2) == 0 ||
        strncmp(dent->d_name, "..", 3) == 0)
      continue;

    file = pdircat(p, dir, dent->d_name, NULL);

    if (pr_fsio_stat(file, &st) < 0)
      continue;
    
    if (S_ISDIR(st.st_mode)) {
      res = site_misc_delete_dir(p, file);
      if (res < 0) {
        int xerrno = errno;

        pr_fsio_closedir(dirh);

        errno = xerrno;
        return -1;
      }

    } else {

      /* Dispatch fake C_DELE command, e.g. for mod_quotatab */
      sub_pool = pr_pool_create_sz(p, 64);
      cmd = pr_cmd_alloc(sub_pool, 2, pstrdup(sub_pool, C_DELE),
        pstrdup(sub_pool, file));
      cmd->arg = pstrdup(cmd->pool, file);
      cmd->cmd_class = CL_WRITE;

      res = pr_cmd_dispatch_phase(cmd, PRE_CMD, 0);
      if (res < 0) {
        int xerrno = errno;

        pr_log_debug(DEBUG3, MOD_SITE_MISC_VERSION
          ": deleting file '%s' blocked by DELE handler: %s", file,
          strerror(xerrno));

        pr_cmd_dispatch_phase(cmd, POST_CMD_ERR, 0);
        pr_cmd_dispatch_phase(cmd, LOG_CMD_ERR, 0);
        pr_response_clear(&resp_err_list);

        destroy_pool(sub_pool);
        pr_fsio_closedir(dirh);

        errno = xerrno;
        return -1;
      }

      res = pr_fsio_unlink(file);
      if (res < 0) {
        int xerrno = errno;

        pr_fsio_closedir(dirh);

        pr_cmd_dispatch_phase(cmd, POST_CMD_ERR, 0);
        pr_cmd_dispatch_phase(cmd, LOG_CMD_ERR, 0);
        pr_response_clear(&resp_err_list);

        destroy_pool(sub_pool);
        pr_fsio_closedir(dirh);

        errno = xerrno;
        return -1;
      }

      pr_cmd_dispatch_phase(cmd, POST_CMD, 0);
      pr_cmd_dispatch_phase(cmd, LOG_CMD, 0);
      pr_response_clear(&resp_list);
      destroy_pool(sub_pool);
    }
  }

  pr_fsio_closedir(dirh);

  /* Dispatch fake C_RMD command, e.g. for mod_quotatab */
  sub_pool = pr_pool_create_sz(p, 64);
  cmd = pr_cmd_alloc(sub_pool, 2, pstrdup(sub_pool, C_RMD),
    pstrdup(sub_pool, dir));
  cmd->arg = pstrdup(cmd->pool, dir);
  cmd->cmd_class = CL_DIRS|CL_WRITE;

  res = pr_cmd_dispatch_phase(cmd, PRE_CMD, 0);
  if (res < 0) {
    int xerrno = errno;

    pr_log_debug(DEBUG3, MOD_SITE_MISC_VERSION
      ": removing directory '%s' blocked by RMD handler: %s", dir,
      strerror(xerrno));

    pr_cmd_dispatch_phase(cmd, POST_CMD_ERR, 0);
    pr_cmd_dispatch_phase(cmd, LOG_CMD_ERR, 0);
    pr_response_clear(&resp_err_list);

    destroy_pool(sub_pool);

    errno = xerrno;
    return -1;
  }

  res = pr_fsio_rmdir(dir);
  if (res < 0) {
    int xerrno = errno;

    pr_cmd_dispatch_phase(cmd, POST_CMD_ERR, 0);
    pr_cmd_dispatch_phase(cmd, LOG_CMD_ERR, 0);
    pr_response_clear(&resp_err_list);

    destroy_pool(sub_pool);
    errno = xerrno;
    return -1;
  }

  pr_cmd_dispatch_phase(cmd, POST_CMD, 0);
  pr_cmd_dispatch_phase(cmd, LOG_CMD, 0);
  pr_response_clear(&resp_list);
  destroy_pool(sub_pool);

  return 0;
}

static int site_misc_delete_path(pool *p, const char *path) {
  struct stat st;

  pr_fs_clear_cache();

  if (pr_fsio_stat(path, &st) < 0)
    return -1;

  if (!S_ISDIR(st.st_mode)) {
    errno = EINVAL;
    return -1;
  }

  return site_misc_delete_dir(p, path);
}

static time_t site_misc_mktime(unsigned int year, unsigned int month,
    unsigned int mday, unsigned int hour, unsigned int min, unsigned int sec) {
  struct tm tm;
  time_t res;
  char *env;

#ifdef HAVE_TZNAME
  char *tzname_dup[2];

  /* The mktime(3) function has a nasty habit of changing the tzname global
   * variable as a side-effect.  This can cause problems, as when the process 
   * has become chrooted, and mktime(3) sets/changes tzname wrong.  (For more
   * information on the tzname global variable, see the tzset(3) man page.)
   *
   * The best way to deal with this issue (which is especially prominent
   * on systems running glibc-2.3 or later, which is particularly ill-behaved
   * in a chrooted environment, as it assumes the ability to find system
   * timezone files at paths which are no longer valid within the chroot)
   * is to set the TZ environment variable explicitly, before starting
   * proftpd.  You can also use the SetEnv configuration directive within
   * the proftpd.conf to set the TZ environment variable, e.g.:
   *
   *  SetEnv TZ PST
   *
   * To try to help sites which fail to do this, the tzname global variable
   * will be copied prior to the mktime(3) call, and the copy restored after
   * the call.  (Note that calling the ctime(3) and localtime(3) functions also
   * causes a similar overwriting/setting of the tzname environment variable.)
   */
  memcpy(&tzname_dup, tzname, sizeof(tzname_dup));
#endif /* HAVE_TZNAME */

  env = pr_env_get(session.pool, "TZ");

  /* Set the TZ environment to be GMT, so that mktime(3) treats the timestamp
   * provided by the client as being in GMT/UTC.
   */
  if (pr_env_set(session.pool, "TZ", "GMT") < 0) {
    pr_log_debug(DEBUG8, MOD_SITE_MISC_VERSION
      ": error setting TZ environment variable to 'GMT': %s", strerror(errno));
  }

  tm.tm_sec = sec;
  tm.tm_min = min;
  tm.tm_hour = hour;
  tm.tm_mday = mday;
  tm.tm_mon = (month - 1);
  tm.tm_year = (year - 1900);
  tm.tm_wday = 0;
  tm.tm_yday = 0;
  tm.tm_isdst = -1;

  res = mktime(&tm);

  /* Restore the old TZ setting, if any. */
  if (env) {
    if (pr_env_set(session.pool, "TZ", env) < 0) {
      pr_log_debug(DEBUG8, MOD_SITE_MISC_VERSION
        ": error setting TZ environment variable to '%s': %s", env,
        strerror(errno));
    }
  }

#ifdef HAVE_TZNAME
  /* Restore the old tzname values prior to returning. */
  memcpy(tzname, tzname_dup, sizeof(tzname_dup));
#endif /* HAVE_TZNAME */

  return res;
}

/* Configuration handlers
 */

/* usage: SiteMiscEngine on|off */
MODRET set_sitemiscengine(cmd_rec *cmd) {
  config_rec *c;
  int bool;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(unsigned int));
  *((unsigned int *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* Command handlers
 */

MODRET site_misc_mkdir(cmd_rec *cmd) {
  if (!site_misc_engine) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc < 2) {
    pr_log_debug(DEBUG5, MOD_SITE_MISC_VERSION
      "%s : wrong number of arguments (%d)", cmd->argv[0], cmd->argc);
    return PR_DECLINED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "MKDIR", 6) == 0) {
    register unsigned int i;
    char *cmd_name, *path = "";
    unsigned char *authenticated;

    if (cmd->argc < 3)
      return PR_DECLINED(cmd);

    authenticated = get_param_ptr(cmd->server->conf, "authenticated", FALSE);

    if (!authenticated ||
        *authenticated == FALSE) {
      pr_response_add_err(R_530, _("Please login with USER and PASS"));
      errno = EACCES;
      return PR_ERROR(cmd);
    }

    for (i = 2; i < cmd->argc; i++)
      path = pstrcat(cmd->tmp_pool, path, *path ? " " : "", cmd->argv[i], NULL);

    path = pr_fs_decode_path(cmd->tmp_pool, path);

    if (site_misc_check_filters(cmd, path) < 0) {
      int xerrno = EPERM;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    path = dir_canonical_path(cmd->tmp_pool, path);
    if (path == NULL) {
      int xerrno = EINVAL;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd_name = cmd->argv[0];
    cmd->argv[0] = "SITE_MKDIR";
    if (!dir_check_canon(cmd->tmp_pool, cmd, G_WRITE, path, NULL)) {
      int xerrno = EPERM;

      cmd->argv[0] = cmd_name;

      pr_log_debug(DEBUG4, MOD_SITE_MISC_VERSION
        ": %s command denied by <Limit>", cmd->argv[0]);
      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
    cmd->argv[0] = cmd_name;

    if (site_misc_create_path(cmd->tmp_pool, path) < 0) {
      int xerrno = errno;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    pr_response_add(R_200, _("SITE %s command successful"), cmd->argv[1]);
    return PR_HANDLED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "HELP", 5) == 0) {
    pr_response_add(R_214, "MKDIR <sp> path");
  }

  return PR_DECLINED(cmd);
}

MODRET site_misc_rmdir(cmd_rec *cmd) {
  if (!site_misc_engine) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc < 2) {
    pr_log_debug(DEBUG5, MOD_SITE_MISC_VERSION
      "%s : wrong number of arguments (%d)", cmd->argv[0], cmd->argc);
    return PR_DECLINED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "RMDIR", 6) == 0) {
    register unsigned int i;
    char *cmd_name, *path = "";
    unsigned char *authenticated;

    if (cmd->argc < 3)
      return PR_DECLINED(cmd);

    authenticated = get_param_ptr(cmd->server->conf, "authenticated", FALSE);

    if (!authenticated ||
        *authenticated == FALSE) {
      pr_response_add_err(R_530, _("Please login with USER and PASS"));
      errno = EACCES;
      return PR_ERROR(cmd);
    }

    for (i = 2; i < cmd->argc; i++)
      path = pstrcat(cmd->tmp_pool, path, *path ? " " : "", cmd->argv[i], NULL);

    path = pr_fs_decode_path(cmd->tmp_pool, path);

    path = dir_canonical_path(cmd->tmp_pool, path);
    if (path == NULL) {
      int xerrno = EINVAL;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd_name = cmd->argv[0];
    cmd->argv[0] = "SITE_RMDIR";
    if (!dir_check_canon(cmd->tmp_pool, cmd, G_WRITE, path, NULL)) {
      int xerrno = EPERM;

      cmd->argv[0] = cmd_name;

      pr_log_debug(DEBUG4, MOD_SITE_MISC_VERSION
        ": %s command denied by <Limit>", cmd->argv[0]);
      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
    cmd->argv[0] = cmd_name;

    if (site_misc_delete_path(cmd->tmp_pool, path) < 0) {
      int xerrno = errno;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    pr_response_add(R_200, _("SITE %s command successful"), cmd->argv[1]);
    return PR_HANDLED(cmd);
  } 

  if (strncasecmp(cmd->argv[1], "HELP", 5) == 0) {
    pr_response_add(R_214, "RMDIR <sp> path");
  }

  return PR_DECLINED(cmd);
}

MODRET site_misc_symlink(cmd_rec *cmd) {
  if (!site_misc_engine) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc < 2) {
    pr_log_debug(DEBUG5, MOD_SITE_MISC_VERSION
      "%s : wrong number of arguments (%d)", cmd->argv[0], cmd->argc);
    return PR_DECLINED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "SYMLINK", 8) == 0) {
    struct stat st;
    int res;
    char *cmd_name, *src, *dst;
    unsigned char *authenticated;

    if (cmd->argc < 4)
      return PR_DECLINED(cmd);

    authenticated = get_param_ptr(cmd->server->conf, "authenticated", FALSE);

    if (!authenticated ||
        *authenticated == FALSE) {
      pr_response_add_err(R_530, _("Please login with USER and PASS"));
      errno = EACCES;
      return PR_ERROR(cmd);
    }

    src = pr_fs_decode_path(cmd->tmp_pool, cmd->argv[2]);
    src = dir_canonical_path(cmd->tmp_pool, src);
    if (src == NULL) {
      int xerrno = EINVAL;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd_name = cmd->argv[0];
    cmd->argv[0] = "SITE_SYMLINK";
    if (!dir_check_canon(cmd->tmp_pool, cmd, G_READ, src, NULL)) {
      int xerrno = EPERM;

      cmd->argv[0] = cmd_name;

      pr_log_debug(DEBUG4, MOD_SITE_MISC_VERSION
        ": %s command denied by <Limit>", cmd->argv[0]);
      pr_response_add_err(R_550, "%s: %s", cmd->argv[2], strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    dst = pr_fs_decode_path(cmd->tmp_pool, cmd->argv[3]);
    dst = dir_canonical_path(cmd->tmp_pool, dst);
    if (dst == NULL) {
      int xerrno = EINVAL;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    if (!dir_check_canon(cmd->tmp_pool, cmd, G_WRITE, dst, NULL)) {
      int xerrno = EPERM;

      cmd->argv[0] = cmd_name;

      pr_log_debug(DEBUG4, MOD_SITE_MISC_VERSION
        ": %s command denied by <Limit>", cmd->argv[0]);
      pr_response_add_err(R_550, "%s: %s", cmd->argv[3], strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
    cmd->argv[0] = cmd_name;

    if (site_misc_check_filters(cmd, dst) < 0) {
      int xerrno = EPERM;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    /* Make sure the source path exists.  The symlink(2) man page suggests
     * that the system call will do this, but experimentally (Mac OSX 10.4)
     * I've seen symlink(2) happily link two names, neither of which exist
     * in the filesystem.
     */
       
    pr_fs_clear_cache();
    res = pr_fsio_stat(src, &st);
    if (res < 0) {
      int xerrno = errno;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    if (pr_fsio_symlink(src, dst) < 0) {
      int xerrno = errno;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    pr_response_add(R_200, _("SITE %s command successful"), cmd->argv[1]);
    return PR_HANDLED(cmd);
  } 

  if (strncasecmp(cmd->argv[1], "HELP", 5) == 0) {
    pr_response_add(R_214, "SYMLINK <sp> source <sp> destination");
  }

  return PR_DECLINED(cmd);
}

MODRET site_misc_utime(cmd_rec *cmd) {
  if (!site_misc_engine) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc < 2) {
    pr_log_debug(DEBUG5, MOD_SITE_MISC_VERSION
      "%s : wrong number of arguments (%d)", cmd->argv[0], cmd->argc);
    return PR_DECLINED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "UTIME", 6) == 0) {
    register unsigned int i;
    char c, *cmd_name, *p, *path = "";
    unsigned int year, month, day, hour, min, sec = 0;
    struct timeval tvs[2];
    unsigned char *authenticated;
    int have_secs_value = FALSE;

    if (cmd->argc < 4)
      return PR_DECLINED(cmd);

    authenticated = get_param_ptr(cmd->server->conf, "authenticated", FALSE);

    if (!authenticated ||
        *authenticated == FALSE) {
      pr_response_add_err(R_530, _("Please login with USER and PASS"));
      errno = EACCES;
      return PR_ERROR(cmd);
    }

    /* Accept both 'YYYYMMDDhhmm' and 'YYYYMMDDhhmmss' formats. */
    if (strlen(cmd->argv[2]) != 12 &&
        strlen(cmd->argv[2]) != 14) {
      int xerrno = EINVAL;

      pr_log_debug(DEBUG7, MOD_SITE_MISC_VERSION
        ": wrong number of digits in timestamp argument '%s' (%lu)",
        cmd->argv[2], (unsigned long) strlen(cmd->argv[2]));
      pr_response_add_err(R_500, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    if (strlen(cmd->argv[2]) == 14) {
      have_secs_value = TRUE;
    }

    for (i = 3; i < cmd->argc; i++)
      path = pstrcat(cmd->tmp_pool, path, *path ? " " : "", cmd->argv[i], NULL);

    path = pr_fs_decode_path(cmd->tmp_pool, path);

    path = dir_canonical_path(cmd->tmp_pool, path);
    if (path == NULL) {
      int xerrno = EINVAL;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd_name = cmd->argv[0];
    cmd->argv[0] = "SITE_UTIME";
    if (!dir_check_canon(cmd->tmp_pool, cmd, G_WRITE, path, NULL)) {
      int xerrno = EPERM;

      cmd->argv[0] = cmd_name;

      pr_log_debug(DEBUG4, MOD_SITE_MISC_VERSION
        ": %s command denied by <Limit>", cmd->argv[0]);
      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
    cmd->argv[0] = cmd_name;

    if (site_misc_check_filters(cmd, path) < 0) {
      int xerrno = EPERM;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    p = cmd->argv[2];
    c = cmd->argv[2][4];
    cmd->argv[2][4] = '\0';
    year = atoi(p);

    cmd->argv[2][4] = c;
    p = &(cmd->argv[2][4]);
    c = cmd->argv[2][6];
    cmd->argv[2][6] = '\0';
    month = atoi(p);

    if (month > 12) {
      int xerrno = EINVAL;

      pr_log_debug(DEBUG7, MOD_SITE_MISC_VERSION
        ": bad number of months in '%s' (%d)", cmd->argv[2], month);
      pr_response_add_err(R_500, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd->argv[2][6] = c;
    p = &(cmd->argv[2][6]);
    c = cmd->argv[2][8];
    cmd->argv[2][8] = '\0';
    day = atoi(p);

    if (day > 31) {
      int xerrno = EINVAL;

      pr_log_debug(DEBUG7, MOD_SITE_MISC_VERSION
        ": bad number of days in '%s' (%d)", cmd->argv[2], day);
      pr_response_add_err(R_500, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd->argv[2][8] = c;
    p = &(cmd->argv[2][8]);
    c = cmd->argv[2][10];
    cmd->argv[2][10] = '\0';
    hour = atoi(p);

    if (hour > 24) {
      int xerrno = EINVAL;

      pr_log_debug(DEBUG7, MOD_SITE_MISC_VERSION
        ": bad number of hours in '%s' (%d)", cmd->argv[2], hour);
      pr_response_add_err(R_500, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    cmd->argv[2][10] = c;
    p = &(cmd->argv[2][10]);

    /* Handle a 'YYYYMMDDhhmmss' argument. */
    if (have_secs_value) {
      c = cmd->argv[2][12];
      cmd->argv[2][12] = '\0';
    }

    min = atoi(p);

    if (min > 60) {
      int xerrno = EINVAL;

      pr_log_debug(DEBUG7, MOD_SITE_MISC_VERSION
        ": bad number of minutes in '%s' (%d)", cmd->argv[2], min);
      pr_response_add_err(R_500, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }

    if (have_secs_value) {
      cmd->argv[2][12] = c;
      p = &(cmd->argv[2][12]);
      sec = atoi(p);

      if (sec > 60) {
        int xerrno = EINVAL;

        pr_log_debug(DEBUG7, MOD_SITE_MISC_VERSION
          ": bad number of seconds in '%s' (%d)", cmd->argv[2], sec);
        pr_response_add_err(R_500, "%s: %s", cmd->arg, strerror(xerrno));

        errno = xerrno;
        return PR_ERROR(cmd);
      }
    }

    tvs[0].tv_usec = tvs[1].tv_usec = 0;
    tvs[0].tv_sec = tvs[1].tv_sec = site_misc_mktime(year, month, day, hour,
      min, sec);

    if (pr_fsio_utimes(path, tvs) < 0) {
      int xerrno = errno;

      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
 
    pr_response_add(R_200, _("SITE %s command successful"), cmd->argv[1]);
    return PR_HANDLED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "HELP", 5) == 0) {
    pr_response_add(R_214, "UTIME <sp> YYYYMMDDhhmm[ss] <sp> path");
  }

  return PR_DECLINED(cmd);
}

/* Initialization functions
 */

static int site_misc_sess_init(void) {
  config_rec *c;

  c = find_config(main_server->conf, CONF_PARAM, "SiteMiscEngine", FALSE);
  if (c) {
    site_misc_engine = *((unsigned int *) c->argv[0]);
  }

  if (!site_misc_engine) {
    return 0;
  }

  /* Advertise support for these SITE commands */
  pr_feat_add("SITE MKDIR");
  pr_feat_add("SITE RMDIR");
  pr_feat_add("SITE SYMLINK");
  pr_feat_add("SITE UTIME");

  return 0;
}

/* Module API tables
 */

static conftable site_misc_conftab[] = {
  { "SiteMiscEngine",	set_sitemiscengine,	NULL },
  { NULL }
};

static cmdtable site_misc_cmdtab[] = {
  { CMD, C_SITE, G_WRITE, site_misc_mkdir,	FALSE,	FALSE, CL_MISC },
  { CMD, C_SITE, G_WRITE, site_misc_rmdir,	FALSE,	FALSE, CL_MISC },
  { CMD, C_SITE, G_WRITE, site_misc_symlink,	FALSE,	FALSE, CL_MISC },
  { CMD, C_SITE, G_WRITE, site_misc_utime,	FALSE,	FALSE, CL_MISC },
  { 0, NULL }
};

module site_misc_module = {
  NULL, NULL,

  /* Module API version 2.0 */
  0x20,

  /* Module name */
  "site_misc",

  /* Module configuration handler table */
  site_misc_conftab,

  /* Module command handler table */
  site_misc_cmdtab,

  /* Module authentication handler table */
  NULL,

  /* Module initialization function */
  NULL,

  /* Session initialization function */
  site_misc_sess_init,

  /* Module version */
  MOD_SITE_MISC_VERSION
};
