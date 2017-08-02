/*
 * ProFTPD - FTP server daemon
 * Copyright (c) 2007-2012 The ProFTPD Project team
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
 */

/* Proctitle management
 * $Id: proctitle.c,v 1.13 2012-04-15 18:04:15 castaglia Exp $
 */

#include "conf.h"

#if PF_ARGV_TYPE == PF_ARGV_PSTAT
# ifdef HAVE_SYS_PSTAT_H
#  include <sys/pstat.h>
# else
#  undef PF_ARGV_TYPE
#  define PF_ARGV_TYPE PF_ARGV_WRITEABLE
# endif /* HAVE_SYS_PSTAT_H */
#endif /* PF_ARGV_PSTAT */

#if PF_ARGV_TYPE == PF_ARGV_PSSTRINGS
# ifndef HAVE_SYS_EXEC_H
#  undef PF_ARGV_TYPE
#  define PF_ARGV_TYPE PF_ARGV_WRITEABLE
# else
#  include <machine/vmparam.h>
#  include <sys/exec.h>
# endif /* HAVE_SYS_EXEC_H */
#endif /* PF_ARGV_PSSTRINGS */

#ifdef HAVE___PROGNAME
extern char *__progname, *__progname_full;
#endif /* HAVE___PROGNAME */
extern char **environ;

#ifndef PR_DEVEL_STACK_TRACE
static char **prog_argv = NULL;
static char *prog_last_argv = NULL;
#endif /* PR_DEVEL_STACK_TRACE */

static int prog_argc = -1;
static char proc_title_buf[BUFSIZ];

static unsigned int proc_flags = 0;
#define PR_PROCTITLE_FL_USE_STATIC		0x001

void pr_proctitle_init(int argc, char *argv[], char *envp[]) {
#ifndef PR_DEVEL_STACK_TRACE
  register int i;
  register size_t envpsize;
  char **p;

  /* Move the environment so setproctitle can use the space. */
  for (i = envpsize = 0; envp[i] != NULL; i++)
    envpsize += strlen(envp[i]) + 1;

  if ((p = (char **) malloc((i + 1) * sizeof(char *))) != NULL) {
    environ = p;

    for (i = 0; envp[i] != NULL; i++) {
      size_t envp_len = strlen(envp[i]);

      if (envp_len > PR_TUNABLE_ENV_MAX) {
        /* Skip any environ variables that are too long. */
        continue;
      }

      environ[i] = malloc(envp_len + 1);
      if (environ[i] != NULL) {
        sstrncpy(environ[i], envp[i], envp_len + 1);
      }
    }

    environ[i] = NULL;
  }

  prog_argv = argv;
  prog_argc = argc;

  for (i = 0; i < prog_argc; i++) {
    if (!i || (prog_last_argv + 1 == argv[i]))
      prog_last_argv = argv[i] + strlen(argv[i]);
  }

  for (i = 0; envp[i] != NULL; i++) {
    if ((prog_last_argv + 1) == envp[i])
      prog_last_argv = envp[i] + strlen(envp[i]);
  }

# ifdef HAVE___PROGNAME
  /* Set the __progname and __progname_full variables so glibc and company
   * don't go nuts.
   */
  __progname = strdup("proftpd");
  __progname_full = strdup(argv[0]);
# endif /* HAVE___PROGNAME */
#else
  /* Silence compiler warning about unused variable when stacktrace
   * developer mode is configured.
   */
  prog_argc = -1;
#endif /* !PR_DEVEL_STACK_TRACE */
  memset(proc_title_buf, '\0', sizeof(proc_title_buf));
}

void pr_proctitle_free(void) {
#ifdef PR_USE_DEVEL
# ifndef PR_DEVEL_STACK_TRACE
  if (environ) {
    register unsigned int i;

    for (i = 0; environ[i] != NULL; i++)
      free(environ[i]);
    free(environ);
    environ = NULL;
  }

#  ifdef HAVE___PROGNAME
  free(__progname);
  __progname = NULL;
  free(__progname_full);
  __progname_full = NULL;
#  endif /* HAVE___PROGNAME */
# endif /* !PR_DEVEL_STACK_TRACE */
#endif /* PR_USE_DEVEL */
}

void pr_proctitle_set_str(const char *str) {
#ifndef PR_DEVEL_STACK_TRACE

# ifndef HAVE_SETPROCTITLE
  char *p;
  int i, procbuflen, maxlen = (prog_last_argv - prog_argv[0]) - 2;

#  if PF_ARGV_TYPE == PF_ARGV_PSTAT
  union pstun pst;
#  endif /* PF_ARGV_PSTAT */

  if (proc_flags & PR_PROCTITLE_FL_USE_STATIC) {
    return;
  }

  sstrncpy(proc_title_buf, str, sizeof(proc_title_buf));
  procbuflen = strlen(proc_title_buf);

#  if PF_ARGV_TYPE == PF_ARGV_NEW
  /* We can just replace argv[] arguments.  Nice and easy. */
  prog_argv[0] = proc_title_buf;
  for (i = 1; i < prog_argc; i++) {
    prog_argv[i] = "";
  }
#  endif /* PF_ARGV_NEW */

#  if PF_ARGV_TYPE == PF_ARGV_WRITEABLE
  /* We can overwrite individual argv[] arguments.  Semi-nice. */
  snprintf(prog_argv[0], maxlen, "%s", proc_title_buf);
  p = &prog_argv[0][procbuflen];

  while (p < prog_last_argv)
    *p++ = '\0';

  for (i = 1; i < prog_argc; i++) {
    prog_argv[i] = "";
  }

#  endif /* PF_ARGV_WRITEABLE */

#  if PF_ARGV_TYPE == PF_ARGV_PSSTRINGS
  PS_STRINGS->ps_nargvstr = 1;
  PS_STRINGS->ps_argvstr = proc_title_buf;
#  endif /* PF_ARGV_PSSTRINGS */

# else
  if (proc_flags & PR_PROCTITLE_FL_USE_STATIC) {
    return;
  }

  setproctitle("%s", str);
# endif /* HAVE_SETPROCTITLE */
#endif /* PR_DEVEL_STACK_TRACE */
}

void pr_proctitle_set(const char *fmt, ...) {
#ifndef PR_DEVEL_STACK_TRACE
  va_list msg;

# ifndef HAVE_SETPROCTITLE
#  if PF_ARGV_TYPE == PF_ARGV_PSTAT
  union pstun pst;
#  endif /* PF_ARGV_PSTAT */
  char *p;
  int i, procbuflen, maxlen = (prog_last_argv - prog_argv[0]) - 2;
# endif /* HAVE_SETPROCTITLE */

  if (proc_flags & PR_PROCTITLE_FL_USE_STATIC) {
    return;
  }

  if (!fmt)
    return;

  va_start(msg, fmt);

  memset(proc_title_buf, 0, sizeof(proc_title_buf));

# ifdef HAVE_SETPROCTITLE
#  if __FreeBSD__ >= 4 && !defined(FREEBSD4_0) && !defined(FREEBSD4_1)
  /* FreeBSD's setproctitle() automatically prepends the process name. */
  vsnprintf(proc_title_buf, sizeof(proc_title_buf), fmt, msg);

#  else /* FREEBSD4 */
  /* Manually append the process name for non-FreeBSD platforms. */
  snprintf(proc_title_buf, sizeof(proc_title_buf), "%s", "proftpd: ");
  vsnprintf(proc_title_buf + strlen(proc_title_buf),
    sizeof(proc_title_buf) - strlen(proc_title_buf), fmt, msg);

#  endif /* FREEBSD4 */
  setproctitle("%s", proc_title_buf);

# else /* HAVE_SETPROCTITLE */
  /* Manually append the process name for non-setproctitle() platforms. */
  snprintf(proc_title_buf, sizeof(proc_title_buf), "%s", "proftpd: ");
  vsnprintf(proc_title_buf + strlen(proc_title_buf),
    sizeof(proc_title_buf) - strlen(proc_title_buf), fmt, msg);

# endif /* HAVE_SETPROCTITLE */

  va_end(msg);

# ifdef HAVE_SETPROCTITLE
  return;
# else
  procbuflen = strlen(proc_title_buf);

#  if PF_ARGV_TYPE == PF_ARGV_NEW
  /* We can just replace argv[] arguments.  Nice and easy. */
  prog_argv[0] = proc_title_buf;
  for (i = 1; i < prog_argc; i++) {
    prog_argv[i] = "";
  }
#  endif /* PF_ARGV_NEW */

#  if PF_ARGV_TYPE == PF_ARGV_WRITEABLE
  /* We can overwrite individual argv[] arguments.  Semi-nice. */
  snprintf(prog_argv[0], maxlen, "%s", proc_title_buf);
  p = &prog_argv[0][procbuflen];

  while (p < prog_last_argv)
    *p++ = '\0';

  for (i = 1; i < prog_argc; i++) {
    prog_argv[i] = "";
  }

#  endif /* PF_ARGV_WRITEABLE */

#  if PF_ARGV_TYPE == PF_ARGV_PSTAT
  pst.pst_command = proc_title_buf;
  pstat(PSTAT_SETCMD, pst, procbuflen, 0, 0);

#  endif /* PF_ARGV_PSTAT */

#  if PF_ARGV_TYPE == PF_ARGV_PSSTRINGS
  PS_STRINGS->ps_nargvstr = 1;
  PS_STRINGS->ps_argvstr = proc_title_buf;
#  endif /* PF_ARGV_PSSTRINGS */

# endif /* HAVE_SETPROCTITLE */
#endif /* !PR_DEVEL_STACK_TRACE */
}

void pr_proctitle_set_static_str(const char *buf) {
  if (buf != NULL) {
    pr_proctitle_set_str(buf);
    proc_flags |= PR_PROCTITLE_FL_USE_STATIC;

  } else {
    /* Reset. */
    proc_flags = 0;
  }
}

int pr_proctitle_get(char *buf, size_t bufsz) {
  if (buf == NULL ||
      bufsz == 0) {

    /* If the caller didn't provide an input buffer, they are simply
     * querying for the length of the current process title.  Easy enough
     * to acquiesce to that request.
     */
    return strlen(proc_title_buf);
  }

  sstrncpy(buf, proc_title_buf, bufsz);
  return strlen(buf);
}
