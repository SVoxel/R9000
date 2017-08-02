/*
 * ProFTPD - FTP server testsuite
 * Copyright (c) 2008-2013 The ProFTPD Project team
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

/* Scoreboard API tests
 * $Id: scoreboard.c,v 1.10 2013-01-05 03:36:39 castaglia Exp $
 */

#include "tests.h"

static pool *p = NULL;

static void set_up(void) {
  if (p == NULL) {
    p = permanent_pool = make_sub_pool(NULL);
  }

  ServerType = SERVER_STANDALONE;
}

static void tear_down(void) {
  if (p) {
    destroy_pool(p);
    p = NULL;
    permanent_pool = NULL;
  } 
}

START_TEST (scoreboard_get_test) {
  const char *ok, *res;

  ok = PR_RUN_DIR "/proftpd.scoreboard";

  res = pr_get_scoreboard();
  fail_unless(res != NULL, "Failed to get scoreboard path: %s",
    strerror(errno));
  fail_unless(strcmp(res, ok) == 0,
    "Expected scoreboard path '%s', got '%s'", ok, res);

  ok = PR_RUN_DIR "/proftpd.scoreboard.lck";

  res = pr_get_scoreboard_mutex();
  fail_unless(res != NULL, "Failed to get scoreboard mutex path: %s",
    strerror(errno));
  fail_unless(strcmp(res, ok) == 0,
    "Expected scoreboard mutex path '%s', got '%s'", ok, res);
}
END_TEST

START_TEST (scoreboard_set_test) {
  int res;
  const char *path;

  res = pr_set_scoreboard(NULL);
  fail_unless(res == -1, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_set_scoreboard("foo");
  fail_unless(res == -1, "Failed to handle non-path argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  res = pr_set_scoreboard("foo/");
  fail_unless(res == -1, "Failed to handle relative path argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  res = pr_set_scoreboard("/foo");
  fail_unless(res == -1, "Failed to handle nonexistent path argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  res = pr_set_scoreboard("/tmp");
  fail_unless(res == -1, "Failed to handle nonexistent path argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  res = pr_set_scoreboard("/tmp/");
  fail_unless(res == -1, "Failed to handle nonexistent path argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  res = mkdir("/tmp/prt-scoreboard", 0777);
  fail_unless(res == 0,
    "Failed to create tmp directory '/tmp/prt-scoreboard': %s",
    strerror(errno));
  res = chmod("/tmp/prt-scoreboard/", 0777);
  fail_unless(res == 0,
    "Failed to create set 0777 perms on '/tmp/prt-scoreboard': %s",
    strerror(errno));

  res = pr_set_scoreboard("/tmp/prt-scoreboard/");
  fail_unless(res == -1, "Failed to handle nonexistent file argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_set_scoreboard("/tmp/prt-scoreboard/bar");
  fail_unless(res == -1, "Failed to handle world-writable path argument");
  fail_unless(errno == EPERM, "Failed to set errno to EPERM");

  res = chmod("/tmp/prt-scoreboard/", 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir("/tmp/prt-scoreboard");
    fail("Failed to set 0775 perms on '/tmp/prt-scoreboard/': %s",
      strerror(xerrno));
  }

  res = pr_set_scoreboard("/tmp/prt-scoreboard/bar");
  fail_unless(res == 0, "Failed to set scoreboard: %s", strerror(errno));
  (void) rmdir("/tmp/prt-scoreboard/");

  path = pr_get_scoreboard();
  fail_unless(path != NULL, "Failed to get scoreboard path: %s",
    strerror(errno));  
  fail_unless(strcmp("/tmp/prt-scoreboard/bar", path) == 0,
    "Expected '%s', got '%s'", "/tmp/prt-scoreboard/bar", path);
}
END_TEST

START_TEST (scoreboard_open_close_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck",
    *symlink_path = "/tmp/prt-scoreboard/symlink";

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  if (symlink(symlink_path, path) == 0) {

    res = pr_open_scoreboard(O_RDWR);
    if (res == 0) {
      (void) unlink(path);
      (void) unlink(mutex_path);
      (void) unlink(symlink_path);
      (void) rmdir(dir);

      fail("Unexpectedly opened symlink scoreboard");
    }

    if (errno != EPERM) {
      int xerrno = errno;

      (void) unlink(symlink_path);
      (void) unlink(mutex_path);
      (void) unlink(path);
      (void) rmdir(dir);

      fail("Failed to set errno to EPERM (got %d)", xerrno);
    }

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) unlink(symlink_path);
  }

  res = pr_open_scoreboard(O_RDONLY);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly opened scoreboard using O_RDONLY");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(symlink_path);
    (void) unlink(mutex_path);
    (void) unlink(path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(mutex_path);
    (void) unlink(path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  /* Now that we have a scoreboard, try opening it again using O_RDONLY. */
  pr_close_scoreboard(FALSE);

  res = pr_open_scoreboard(O_RDONLY);
  if (res == 0) {
    (void) unlink(mutex_path);
    (void) unlink(path);
    (void) rmdir(dir);

    fail("Unexpectedly opened scoreboard using O_RDONLY");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(mutex_path);
    (void) unlink(path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  (void) unlink(mutex_path);
  (void) unlink(path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_delete_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";
  struct stat st;

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(mutex_path);
    (void) unlink(path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = stat(pr_get_scoreboard(), &st);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(mutex_path);
    (void) unlink(path);
    (void) rmdir(dir);

    fail("Failed to stat scoreboard: %s", strerror(xerrno));
  }

  pr_delete_scoreboard();

  res = stat(pr_get_scoreboard(), &st);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly found deleted scoreboard");
  }

  res = stat(pr_get_scoreboard_mutex(), &st);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly found deleted scoreboard mutex");
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_restore_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_restore_scoreboard();
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly restored scoreboard");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = pr_restore_scoreboard();
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("restoring scoreboard before rewind succeeded unexpectedly");
  }

  res = pr_rewind_scoreboard();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to rewind scoreboard: %s", strerror(xerrno));
  }

  res = pr_restore_scoreboard();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to restore scoreboard: %s", strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_rewind_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_rewind_scoreboard();
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly rewound scoreboard");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = pr_rewind_scoreboard();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to rewind scoreboard: %s", strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_scrub_test) {
  uid_t euid;
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_scoreboard_scrub();
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly scrubbed scoreboard");
  }

  euid = geteuid();
  if (euid != 0) {
    if (errno != EPERM &&
        errno != ENOENT) {
      int xerrno = errno;

      (void) unlink(path);
      (void) unlink(mutex_path);
      (void) rmdir(dir);

      fail("Failed to set errno to EPERM/ENOENT, got %d [%s] (euid = %lu)",
        xerrno, strerror(xerrno), (unsigned long) euid);
    }

  } else {
    if (errno != ENOENT) {
      int xerrno = errno;

      (void) unlink(path);
      (void) unlink(mutex_path);
      (void) rmdir(dir);

      fail("Failed to set errno to ENOENT, got %d [%s] (euid = %lu)", xerrno,
        strerror(xerrno), (unsigned long) euid);
    }
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_scrub();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to scrub scoreboard: %s", strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_get_daemon_pid_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";
  pid_t daemon_pid;

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  daemon_pid = pr_scoreboard_get_daemon_pid();
  if (daemon_pid != getpid()) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Expected %lu, got %lu", (unsigned long) getpid(),
      (unsigned long) daemon_pid);
  }

  pr_delete_scoreboard();

  ServerType = SERVER_INETD;

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  daemon_pid = pr_scoreboard_get_daemon_pid();
  if (daemon_pid != 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Expected %lu, got %lu", (unsigned long) 0,
      (unsigned long) daemon_pid);
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_get_daemon_uptime_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";
  time_t daemon_uptime, now;

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  daemon_uptime = pr_scoreboard_get_daemon_uptime();
  now = time(NULL);

  if (daemon_uptime > now) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Expected %lu, got %lu", (unsigned long) now,
      (unsigned long) daemon_uptime);
  }

  pr_delete_scoreboard();

  ServerType = SERVER_INETD;

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  daemon_uptime = pr_scoreboard_get_daemon_uptime();
  if (daemon_uptime != 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Expected %lu, got %lu", (unsigned long) 0,
      (unsigned long) daemon_uptime);
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_entry_add_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_scoreboard_entry_add();
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly added entry to scoreboard");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_add();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to add entry to scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_add();
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly added entry to scoreboard");
  }

  if (errno != EPERM) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EPERM (got %d)", xerrno);
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_entry_del_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_scoreboard_entry_del(FALSE);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly deleted entry from scoreboard");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_del(FALSE);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly deleted entry from scoreboard");
  }

  if (errno != ENOENT) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to ENOENT (got %d)", xerrno);
  }

  res = pr_scoreboard_entry_add();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to add entry to scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_del(FALSE);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to delete entry from scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_del(FALSE);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly deleted entry from scoreboard");
  }

  if (errno != ENOENT) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to ENOENT (got %d)", xerrno);
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_entry_read_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";
  pr_scoreboard_entry_t *score;

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  score = pr_scoreboard_entry_read();
  if (score != NULL) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly read scoreboard entry");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  /* We expect NULL here because the scoreboard file should be empty. */
  score = pr_scoreboard_entry_read();
  if (score != NULL) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly read scoreboard entry");
  }

  res = pr_scoreboard_entry_add();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to add entry to scoreboard: %s", strerror(xerrno));
  }

  score = pr_scoreboard_entry_read();
  if (score == NULL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to read scoreboard entry: %s", strerror(xerrno));
  }

  if (score->sce_pid != getpid()) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to read expected scoreboard entry (expected PID %lu, got %lu)",
      (unsigned long) getpid(), (unsigned long) score->sce_pid);
  }

  score = pr_scoreboard_entry_read();
  if (score != NULL) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly read scoreboard entry");
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_entry_get_test) {
  int res;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";
  const char *val;

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  val = pr_scoreboard_entry_get(-1);
  if (val != NULL) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly read value from scoreboard entry");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  val = pr_scoreboard_entry_get(-1);
  if (val != NULL) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly read value from scoreboard entry");
  }

  if (errno != EPERM) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EPERM (got %d)", xerrno);
  }

  res = pr_scoreboard_entry_add();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to add entry to scoreboard: %s", strerror(xerrno));
  }

  val = pr_scoreboard_entry_get(-1);
  if (val != NULL) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly read value from scoreboard entry");
  }

  if (errno != ENOENT) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to ENOENT (got %d)", xerrno);
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_entry_update_test) {
  int res;
  const char *val;
  const char *dir = "/tmp/prt-scoreboard/", *path = "/tmp/prt-scoreboard/test",
    *mutex_path = "/tmp/prt-scoreboard/test.lck";
  pid_t pid = getpid();

  res = mkdir(dir, 0775);
  fail_unless(res == 0, "Failed to create directory '%s': %s", dir,
    strerror(errno));

  res = chmod(dir, 0775);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set perms on '%s' to 0775': %s", dir, strerror(xerrno));
  }

  res = pr_set_scoreboard(path);
  if (res < 0) {
    int xerrno = errno;

    (void) rmdir(dir);
    fail("Failed to set scoreboard to '%s': %s", path, strerror(xerrno));
  }

  (void) unlink(path);
  (void) unlink(mutex_path);

  res = pr_scoreboard_entry_update(pid, 0);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly updated scoreboard entry");
  }

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  res = pr_open_scoreboard(O_RDWR);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to open scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_update(pid, 0);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly updated scoreboard entry");
  }

  if (errno != EPERM) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to EPERM (got %d)", xerrno);
  }

  res = pr_scoreboard_entry_add();
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to add entry to scoreboard: %s", strerror(xerrno));
  }

  res = pr_scoreboard_entry_update(pid, -1);
  if (res == 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Unexpectedly updated scoreboard entry");
  }

  if (errno != ENOENT) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to set errno to ENOENT (got %d)", xerrno);
  }

  val = "cwd";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_CWD, val, NULL);
  if (res < 0) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to update PR_SCORE_CWD: %s", strerror(xerrno));
  }
 
  val = pr_scoreboard_entry_get(PR_SCORE_CWD); 
  if (val == NULL) {
    int xerrno = errno;

    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Failed to get entry PR_SCORE_CWD: %s", strerror(xerrno));
  }

  if (strcmp(val, "cwd") != 0) {
    (void) unlink(path);
    (void) unlink(mutex_path);
    (void) rmdir(dir);

    fail("Expected '%s', got '%s'", "cwd", val);
  }

  (void) unlink(path);
  (void) unlink(mutex_path);
  (void) rmdir(dir);
}
END_TEST

START_TEST (scoreboard_disabled_test) {
  register unsigned int i = 0;
  const char *paths[4] = {
    "/dev/null",
    "none",
    "off",
    NULL
  };
  const char *path;

  for (path = paths[i]; path != NULL; path = paths[i++]) { 
    int res;
    const char *field, *ok;
    pid_t scoreboard_pid;
    time_t scoreboard_uptime;
    pr_scoreboard_entry_t *score;

    res = pr_set_scoreboard(path);
    fail_unless(res == 0, "Failed set to scoreboard to '%s': %s", path,
      strerror(errno));

    ok = PR_RUN_DIR "/proftpd.scoreboard";

    path = pr_get_scoreboard();
    fail_unless(path != NULL, "Failed to get scoreboard path: %s",
      strerror(errno));
    fail_unless(strcmp(path, ok) == 0,
      "Expected path '%s', got '%s'", ok, path);

    res = pr_open_scoreboard(O_RDONLY);
    fail_unless(res == 0, "Failed to open '%s' scoreboard: %s", path,
      strerror(errno));

    res = pr_scoreboard_scrub();
    fail_unless(res == 0, "Failed to scrub '%s' scoreboard: %s", path,
      strerror(errno));

    scoreboard_pid = pr_scoreboard_get_daemon_pid();
    fail_unless(scoreboard_pid == 0,
      "Expected to get scoreboard PID 0, got %lu",
      (unsigned long) scoreboard_pid);

    scoreboard_uptime = pr_scoreboard_get_daemon_uptime();
    fail_unless(scoreboard_uptime == 0,
      "Expected to get scoreboard uptime 0, got %lu",
      (unsigned long) scoreboard_uptime);

    res = pr_scoreboard_entry_add();
    fail_unless(res == 0, "Failed to add entry to '%s' scoreboard: %s", path,
      strerror(errno));

    score = pr_scoreboard_entry_read();
    fail_unless(score == NULL, "Expected null entry");

    field = pr_scoreboard_entry_get(PR_SCORE_CMD_ARG);
    fail_unless(field == NULL, "Expected null CMD_ARG field");

    res = pr_scoreboard_entry_update(getpid(), PR_SCORE_CWD, "foo", NULL);
    fail_unless(res == 0, "Failed to update CWD field: %s", strerror(errno));

    res = pr_scoreboard_entry_del(FALSE);
    fail_unless(res == 0, "Failed to delete entry from '%s' scoreboard: %s",
      path, strerror(errno));

    res = pr_close_scoreboard(FALSE);
    fail_unless(res == 0, "Failed to close '%s' scoreboard: %s", path,
      strerror(errno));

    /* Internal hack: even calling pr_set_scoreboard() with a NULL
     * argument will set the Scoreboard API internal flag back to true.
     */
    pr_set_scoreboard(NULL);
  }
}
END_TEST

Suite *tests_get_scoreboard_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("scoreboard");

  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, scoreboard_get_test);
  tcase_add_test(testcase, scoreboard_set_test);
  tcase_add_test(testcase, scoreboard_open_close_test);
  tcase_add_test(testcase, scoreboard_delete_test);
  tcase_add_test(testcase, scoreboard_restore_test);
  tcase_add_test(testcase, scoreboard_rewind_test);
  tcase_add_test(testcase, scoreboard_scrub_test);
  tcase_add_test(testcase, scoreboard_get_daemon_pid_test);
  tcase_add_test(testcase, scoreboard_get_daemon_uptime_test);
  tcase_add_test(testcase, scoreboard_entry_add_test);
  tcase_add_test(testcase, scoreboard_entry_del_test);
  tcase_add_test(testcase, scoreboard_entry_read_test);
  tcase_add_test(testcase, scoreboard_entry_get_test);
  tcase_add_test(testcase, scoreboard_entry_update_test);
  tcase_add_test(testcase, scoreboard_disabled_test);

  suite_add_tcase(suite, testcase);

  return suite;
}
