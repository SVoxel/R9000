/*
 * ProFTPD - FTP server daemon
 * Copyright (c) 2009-2011 The ProFTPD Project team
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
 * $Id: filter.c,v 1.8 2011-12-20 22:56:48 castaglia Exp $
 */

#include "conf.h"

static const char *trace_channel = "filter";

int pr_filter_allow_path(xaset_t *set, const char *path) {
#ifdef PR_USE_REGEX
  pr_regex_t *pre;
  int res;

  /* Check any relevant PathAllowFilter first. */

  pre = get_param_ptr(set, "PathAllowFilter", FALSE);
  if (pre) {
    res = pr_regexp_exec(pre, path, 0, NULL, 0, 0, 0);
    if (res != 0) {
      return PR_FILTER_ERR_FAILS_ALLOW_FILTER;
    }

    pr_trace_msg(trace_channel, 8, "'%s' allowed by PathAllowFilter '%s'", path,
      pr_regexp_get_pattern(pre));
  }

  /* Next check any applicable PathDenyFilter. */

  pre = get_param_ptr(CURRENT_CONF, "PathDenyFilter", FALSE);
  if (pre) {
    res = pr_regexp_exec(pre, path, 0, NULL, 0, 0, 0);
    if (res == 0) {
      return PR_FILTER_ERR_FAILS_DENY_FILTER;
    } 

    pr_trace_msg(trace_channel, 8, "'%s' allowed by PathDenyFilter '%s'", path,
      pr_regexp_get_pattern(pre));
  }

  return 0;
#else
  return 0;
#endif
}

int pr_filter_parse_flags(pool *p, const char *flags_str) {
  if (p == NULL ||
      flags_str == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (flags_str[0] != '[' ||
      flags_str[strlen(flags_str)-1] != ']') {
    errno = EINVAL;
    return -1;
  }

  /* Right now, we only support "[NC]", for "no case", i.e. REG_ICASE. */
  if (strncmp(flags_str, "[NC]", 5) == 0 ||
      strncmp(flags_str, "[nocase]", 9) == 0) {
    return REG_ICASE;
  }

  return 0;
}
