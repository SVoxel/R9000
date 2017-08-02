/*
 * ProFTPD - FTP server testsuite
 * Copyright (c) 2008-2014 The ProFTPD Project team
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

/* NetAddr API tests
 * $Id: netaddr.c,v 1.13 2014-01-27 18:31:35 castaglia Exp $
 */

#include "tests.h"

static pool *p = NULL;

/* Fixtures */

static void set_up(void) {
  if (p == NULL) {
    p = permanent_pool = make_sub_pool(NULL);
  }

  init_netaddr();
}

static void tear_down(void) {
  if (p) {
    destroy_pool(p);
    p = NULL;
    permanent_pool = NULL;
  } 
}

/* Tests */

START_TEST (netaddr_alloc_test) {
  pr_netaddr_t *res;

  res = pr_netaddr_alloc(NULL);
  fail_unless(res == NULL, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_netaddr_alloc(p);
  fail_unless(res != NULL, "Failed to allocate netaddr: %s", strerror(errno));
  fail_unless(res->na_family == 0, "Allocated netaddr is not zeroed");
}
END_TEST

START_TEST (netaddr_dup_test) {
  pr_netaddr_t *res, *addr;

  res = pr_netaddr_dup(NULL, NULL);
  fail_unless(res == NULL, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_netaddr_dup(p, NULL);
  fail_unless(res == NULL, "Failed to handle null addr");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_alloc(p);
  pr_netaddr_set_family(addr, AF_INET);
  
  res = pr_netaddr_dup(NULL, addr);
  fail_unless(res == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_netaddr_dup(p, addr);
  fail_unless(res != NULL, "Failed to dup netaddr: %s", strerror(errno));
  fail_unless(res->na_family == addr->na_family, "Expected family %d, got %d",
    addr->na_family, res->na_family);
}
END_TEST

START_TEST (netaddr_clear_test) {
  pr_netaddr_t *addr;

  mark_point();
  pr_netaddr_clear(NULL);

  addr = pr_netaddr_alloc(p);
  addr->na_family = 1;

  pr_netaddr_clear(addr);
  fail_unless(addr->na_family == 0, "Failed to clear addr");
}
END_TEST

START_TEST (netaddr_get_addr_test) {
  pr_netaddr_t *res;
  const char *name;
  array_header *addrs = NULL;

  res = pr_netaddr_get_addr(NULL, NULL, NULL);
  fail_unless(res == NULL, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_netaddr_get_addr(p, NULL, NULL);
  fail_unless(res == NULL, "Failed to handle null name");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  name = "127.0.0.1";

  res = pr_netaddr_get_addr(NULL, name, NULL);
  fail_unless(res == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(res != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  fail_unless(res->na_family == AF_INET, "Expected family %d, got %d",
    AF_INET, res->na_family);

  name = "localhost";

  res = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(res != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  fail_unless(res->na_family == AF_INET, "Expected family %d, got %d",
    AF_INET, res->na_family);

  res = pr_netaddr_get_addr(p, name, &addrs);
  fail_unless(res != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  fail_unless(res->na_family == AF_INET, "Expected family %d, got %d",
    AF_INET, res->na_family);
}
END_TEST

START_TEST (netaddr_get_addr2_test) {
  pr_netaddr_t *res;
  const char *name;
  int flags;

  flags = PR_NETADDR_GET_ADDR_FL_INCL_DEVICE;
  name = "lo0";
  res = pr_netaddr_get_addr2(p, name, NULL, flags);
  if (res == NULL) {
    /* Fallback to using a device name of "lo". */
    name = "lo";
    res = pr_netaddr_get_addr2(p, name, NULL, flags);
  }

  fail_if(res == NULL,
    "Expected to resolve name '%s' to an address via INCL_DEVICE", name);

  flags = PR_NETADDR_GET_ADDR_FL_EXCL_DNS;
  name = "localhost";
  res = pr_netaddr_get_addr2(p, name, NULL, flags);
  fail_unless(res == NULL, "Resolved name '%s' to IP address unexpectedly",
    name);
  fail_unless(errno == ENOENT, "Failed to set errno to ENOENT");
}
END_TEST

START_TEST (netaddr_get_family_test) {
  pr_netaddr_t *addr;
  int res;

  res = pr_netaddr_get_family(NULL);
  fail_unless(res == -1, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, "localhost", NULL);
  fail_unless(addr != NULL, "Failed to get addr for 'localhost': %s",
    strerror(errno));

  res = pr_netaddr_get_family(addr);
  fail_unless(res == AF_INET, "Expected family %d, got %d", AF_INET,
    res);
}
END_TEST

START_TEST (netaddr_set_family_test) {
  pr_netaddr_t *addr;
  int res;

  res = pr_netaddr_set_family(NULL, 0);
  fail_unless(res == -1, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, "127.0.0.1", NULL);
  fail_unless(addr != NULL, "Failed to get addr for '127.0.0.1': %s",
    strerror(errno));

  res = pr_netaddr_set_family(addr, -1);
  fail_unless(res == -1, "Failed to handle bad family");
#ifdef EAFNOSUPPORT
  fail_unless(errno == EAFNOSUPPORT, "Failed to set errno to EAFNOSUPPORT");
#else
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");
#endif

  res = pr_netaddr_set_family(addr, AF_INET);
  fail_unless(res == 0, "Failed to set family to AF_INET: %s", strerror(errno));
}
END_TEST

START_TEST (netaddr_cmp_test) {
}
END_TEST

START_TEST (netaddr_ncmp_test) {
}
END_TEST

START_TEST (netaddr_fnmatch_test) {
}
END_TEST

START_TEST (netaddr_get_sockaddr_test) {
}
END_TEST

START_TEST (netaddr_get_sockaddr_len_test) {
}
END_TEST

START_TEST (netaddr_set_sockaddr_test) {
}
END_TEST

START_TEST (netaddr_set_sockaddr_any_test) {
}
END_TEST

START_TEST (netaddr_get_inaddr_test) {
}
END_TEST

START_TEST (netaddr_get_inaddr_len_test) {
}
END_TEST

START_TEST (netaddr_get_port_test) {
  pr_netaddr_t *addr;
  unsigned int res;

  res = pr_netaddr_get_port(NULL);
  fail_unless(res == 0, "Failed to handle null addr");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, "127.0.0.1", NULL);
  fail_unless(addr != NULL, "Failed to get addr for '127.0.0.1': %s",
    strerror(errno));

  res = pr_netaddr_get_port(addr);
  fail_unless(res == 0, "Expected port %u, got %u", 0, res);

  addr->na_family = -1;
  res = pr_netaddr_get_port(addr);
  fail_unless(res == 0, "Expected port %u, got %u", 0, res);
  fail_unless(errno == EPERM, "Failed to set errno to EPERM");
}
END_TEST

START_TEST (netaddr_set_port_test) {
  pr_netaddr_t *addr;
  unsigned int port;
  int res;

  res = pr_netaddr_set_port(NULL, 0);
  fail_unless(res == -1, "Failed to handle null addr");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, "127.0.0.1", NULL);
  fail_unless(addr != NULL, "Failed to get addr for '127.0.0.1': %s",
    strerror(errno));

  addr->na_family = -1;
  res = pr_netaddr_set_port(addr, 1);
  fail_unless(res == -1, "Failed to handle bad family");
  fail_unless(errno == EPERM, "Failed to set errno to EPERM");

  addr->na_family = AF_INET;
  res = pr_netaddr_set_port(addr, 1);
  fail_unless(res == 0, "Failed to set port: %s", strerror(errno));

  port = pr_netaddr_get_port(addr);
  fail_unless(port == 1, "Expected port %u, got %u", 1, port);
}
END_TEST

START_TEST (netaddr_set_reverse_dns_test) {
  int res;

  res = pr_netaddr_set_reverse_dns(FALSE);
  fail_unless(res == 1, "Expected reverse %d, got %d", 1, res);

  res = pr_netaddr_set_reverse_dns(TRUE);
  fail_unless(res == 0, "Expected reverse %d, got %d", 0, res);
}
END_TEST

START_TEST (netaddr_get_dnsstr_test) {
  pr_netaddr_t *addr;
  const char *ip, *res;

  ip = "127.0.0.1";

  res = pr_netaddr_get_dnsstr(NULL);
  fail_unless(res == NULL, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, ip, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", ip,
    strerror(errno));

  pr_netaddr_set_reverse_dns(FALSE);

  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, ip) == 0, "Expected '%s', got '%s'", ip, res);

  pr_netaddr_set_reverse_dns(TRUE);

  /* Even though we should expect a DNS name, not an IP address, the
   * previous call to pr_netaddr_get_dnsstr() cached the IP address.
   */
  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, ip) == 0, "Expected '%s', got '%s'", ip, res);

  pr_netaddr_clear(addr);

  /* Clearing the address doesn't work, since that removes even the address
   * info, in addition to the cached strings.
   */
  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, "") == 0, "Expected '%s', got '%s'", "", res);

  /* We need to clear the netaddr internal cache as well. */
  pr_netaddr_clear_cache();
  addr = pr_netaddr_get_addr(p, ip, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", ip,
    strerror(errno));

  mark_point();
  fail_unless(addr->na_have_dnsstr == 0, "addr already has cached DNS str");

  mark_point();
  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));

  mark_point();

  /* Depending on the contents of /etc/hosts, resolving 127.0.0.1 could
   * return either "localhost" or "localhost.localdomain".  Perhaps even
   * other variations, although these should be the most common.
   */
  fail_unless(strcmp(res, "localhost") == 0 ||
              strcmp(res, "localhost.localdomain") == 0,
    "Expected '%s', got '%s'", "localhost or localhost.localdomain", res);
}
END_TEST

#ifdef PR_USE_IPV6
START_TEST (netaddr_get_dnsstr_ipv6_test) {
  pr_netaddr_t *addr;
  const char *ip, *res;

  ip = "::1";

  res = pr_netaddr_get_dnsstr(NULL);
  fail_unless(res == NULL, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, ip, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", ip,
    strerror(errno));

  pr_netaddr_set_reverse_dns(FALSE);

  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, ip) == 0, "Expected '%s', got '%s'", ip, res);

  pr_netaddr_set_reverse_dns(TRUE);

  /* Even though we should expect a DNS name, not an IP address, the
   * previous call to pr_netaddr_get_dnsstr() cached the IP address.
   */
  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, ip) == 0, "Expected '%s', got '%s'", ip, res);

  pr_netaddr_clear(addr);

  /* Clearing the address doesn't work, since that removes even the address
   * info, in addition to the cached strings.
   */
  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, "") == 0, "Expected '%s', got '%s'", "", res);

  /* We need to clear the netaddr internal cache as well. */
  pr_netaddr_clear_cache();
  addr = pr_netaddr_get_addr(p, ip, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", ip,
    strerror(errno));

  mark_point();
  fail_unless(addr->na_have_dnsstr == 0, "addr already has cached DNS str");

  mark_point();
  res = pr_netaddr_get_dnsstr(addr);
  fail_unless(res != NULL, "Failed to get DNS str for addr: %s",
    strerror(errno));

  mark_point();

  /* Depending on the contents of /etc/hosts, resolving ::1 could
   * return either "localhost" or "localhost.localdomain".  Perhaps even
   * other variations, although these should be the most common.
   */
  fail_unless(strcmp(res, "localhost") == 0 ||
              strcmp(res, "localhost.localdomain") == 0 ||
              strcmp(res, "localhost6") == 0 ||
              strcmp(res, "localhost6.localdomain") == 0 ||
              strcmp(res, "ip6-localhost") == 0 ||
              strcmp(res, "ip6-loopback") == 0 ||
              strcmp(res, ip) == 0,
    "Expected '%s', got '%s'", "localhost, localhost.localdomain et al", res);
}
END_TEST
#endif /* PR_USE_IPV6 */

START_TEST (netaddr_get_ipstr_test) {
  pr_netaddr_t *addr;
  const char *res;

  res = pr_netaddr_get_ipstr(NULL);
  fail_unless(res == NULL, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  addr = pr_netaddr_get_addr(p, "localhost", NULL);
  fail_unless(addr != NULL, "Failed to get addr for 'localhost': %s",
    strerror(errno));

  res = pr_netaddr_get_ipstr(addr);
  fail_unless(res != NULL, "Failed to get IP str for addr: %s",
    strerror(errno));
  fail_unless(strcmp(res, "127.0.0.1") == 0, "Expected '%s', got '%s'",
    "127.0.0.1", res);
  fail_unless(addr->na_have_ipstr == 1, "addr should have cached IP str");

  pr_netaddr_clear(addr);
  res = pr_netaddr_get_ipstr(addr);
  fail_unless(res == NULL, "Expected null, got '%s'", res);

}
END_TEST

START_TEST (netaddr_validate_dns_str_test) {
  char *res, *str;

  res = pr_netaddr_validate_dns_str(NULL);
  fail_unless(res == NULL, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  str = pstrdup(p, "foo");
  res = pr_netaddr_validate_dns_str(str);
  fail_unless(strcmp(res, str) == 0, "Expected '%s', got '%s'", str, res);

  str = pstrdup(p, "[foo]");
  res = pr_netaddr_validate_dns_str(str);
  fail_unless(strcmp(res, "_foo_") == 0, "Expected '%s', got '%s'",
    "_foo_", res);

  str = pstrdup(p, "foo.");
  res = pr_netaddr_validate_dns_str(str);
  fail_unless(strcmp(res, str) == 0, "Expected '%s', got '%s'",
    str, res);

  str = pstrdup(p, "foo:");
  res = pr_netaddr_validate_dns_str(str);
#ifdef PR_USE_IPV6
  fail_unless(strcmp(res, str) == 0, "Expected '%s', got '%s'",
    str, res);
#else
  fail_unless(strcmp(res, "foo_") == 0, "Expected '%s', got '%s'",
    "foo_", res);
#endif
}
END_TEST

START_TEST (netaddr_get_localaddr_str_test) {
  const char *res;

  res = pr_netaddr_get_localaddr_str(NULL);
  fail_unless(res == NULL, "Failed to handle null argument");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_netaddr_get_localaddr_str(p);
  fail_unless(res != NULL, "Failed to get local addr: %s", strerror(errno));
}
END_TEST

START_TEST (netaddr_is_v4_test) {
  int res;
  const char *name;

  res = pr_netaddr_is_v4(NULL);
  fail_unless(res == -1, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  name = "::1";
  res = pr_netaddr_is_v4(name);
  fail_unless(res == FALSE, "Expected 'false' for IPv6 address '%s', got %d",
    name, res);

  name = "localhost";
  res = pr_netaddr_is_v4(name);
  fail_unless(res == FALSE, "Expected 'false' for DNS name '%s', got %d",
    name, res);

  name = "127.0.0.1";
  res = pr_netaddr_is_v4(name);
  fail_unless(res == TRUE, "Expected 'true' for IPv4 address '%s', got %d",
    name, res);
}
END_TEST

START_TEST (netaddr_is_v6_test) {
  int res;
  const char *name;

  res = pr_netaddr_is_v6(NULL);
  fail_unless(res == -1, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  name = "127.0.0.1";
  res = pr_netaddr_is_v6(name);
  fail_unless(res == FALSE, "Expected 'false' for IPv4 address '%s', got %d",
    name, res);

  name = "localhost";
  res = pr_netaddr_is_v6(name);
  fail_unless(res == FALSE, "Expected 'false' for DNS name '%s', got %d",
    name, res);

  pr_netaddr_enable_ipv6();

  if (pr_netaddr_use_ipv6() == TRUE) {
    name = "::1";
    res = pr_netaddr_is_v6(name);
    fail_unless(res == TRUE, "Expected 'true' for IPv6 address '%s', got %d",
      name, res);
  }
}
END_TEST

START_TEST (netaddr_is_v4mappedv6_test) {
  int res;
  const char *name;
  pr_netaddr_t *addr;

  res = pr_netaddr_is_v4mappedv6(NULL);
  fail_unless(res == -1, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  name = "127.0.0.1";
  addr = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_v4mappedv6(addr);
  fail_unless(res == -1, "Expected -1 for IPv4 address '%s', got %d",
    name, res);
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL; got %d [%s]",
    errno, strerror(errno));

  name = "::1";
  addr = pr_netaddr_get_addr(p, name, NULL);
#ifdef PR_USE_IPV6
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_v4mappedv6(addr);
  fail_unless(res == FALSE, "Expected 'false' for IPv6 address '%s', got %d",
    name, res);
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL; got %d [%s]",
    errno, strerror(errno));
#else
  fail_unless(addr == NULL,
    "IPv6 support disabled, should not be able to get addr for '%s'", name);
#endif /* PR_USE_IPV6 */

  name = "::ffff:127.0.0.1";
  addr = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_v4mappedv6(addr);
#ifdef PR_USE_IPV6
  fail_unless(res == TRUE,
    "Expected 'true' for IPv4-mapped IPv6 address '%s', got %d", name, res);
#else
  fail_unless(res == -1,
    "Expected -1 for IPv4-mapped IPv6 address '%s' (--disable-ipv6 used)");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL; got %d [%s]",
    errno, strerror(errno));
#endif /* PR_USE_IPV6 */
}
END_TEST

START_TEST (netaddr_is_rfc1918_test) {
  int res;
  const char *name;
  pr_netaddr_t *addr;

  res = pr_netaddr_is_rfc1918(NULL);
  fail_unless(res == -1, "Failed to handle null arguments");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");

  name = "127.0.0.1";
  addr = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_rfc1918(addr);
  fail_unless(res == FALSE, "Failed to handle non-RFC1918 IPv4 address");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL, got %s (%d)",
    strerror(errno), errno);

  name = "::1";
  addr = pr_netaddr_get_addr(p, name, NULL);
#ifdef PR_USE_IPV6
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_rfc1918(addr);
  fail_unless(res == FALSE, "Failed to handle IPv6 address");
  fail_unless(errno == EINVAL, "Failed to set errno to EINVAL");
#else
  fail_unless(addr == NULL,
    "IPv6 support disabled, should not be able to get addr for '%s'", name);
#endif /* PR_USE_IPV6 */

  name = "10.0.0.1";
  addr = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_rfc1918(addr);
  fail_unless(res == TRUE, "Expected 'true' for address '%s'", name);

  name = "192.168.0.1";
  addr = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_rfc1918(addr);
  fail_unless(res == TRUE, "Expected 'true' for address '%s'", name);

  name = "172.31.200.55";
  addr = pr_netaddr_get_addr(p, name, NULL);
  fail_unless(addr != NULL, "Failed to get addr for '%s': %s", name,
    strerror(errno));
  res = pr_netaddr_is_rfc1918(addr);
  fail_unless(res == TRUE, "Expected 'true' for address '%s'", name);
}
END_TEST

START_TEST (netaddr_disable_ipv6_test) {
  unsigned char use_ipv6;

  use_ipv6 = pr_netaddr_use_ipv6();

#ifdef PR_USE_IPV6
  fail_unless(use_ipv6 == TRUE, "Expected %d, got %d", TRUE, use_ipv6);
#else
  fail_unless(use_ipv6 == FALSE, "Expected %d, got %d", FALSE, use_ipv6);
#endif

  pr_netaddr_disable_ipv6();

  use_ipv6 = pr_netaddr_use_ipv6();
  fail_unless(use_ipv6 == FALSE, "Expected %d, got %d", FALSE, use_ipv6);
}
END_TEST

START_TEST (netaddr_enable_ipv6_test) {
  unsigned char use_ipv6;

  pr_netaddr_enable_ipv6();

  use_ipv6 = pr_netaddr_use_ipv6();
#ifdef PR_USE_IPV6
  fail_unless(use_ipv6 == TRUE, "Expected %d, got %d", TRUE, use_ipv6);
#else
  fail_unless(use_ipv6 == FALSE, "Expected %d, got %d", FALSE, use_ipv6);
#endif
}
END_TEST

Suite *tests_get_netaddr_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("netaddr");

  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, netaddr_alloc_test);
  tcase_add_test(testcase, netaddr_dup_test);
  tcase_add_test(testcase, netaddr_clear_test);
  tcase_add_test(testcase, netaddr_get_addr_test);
  tcase_add_test(testcase, netaddr_get_addr2_test);
  tcase_add_test(testcase, netaddr_get_family_test);
  tcase_add_test(testcase, netaddr_set_family_test);
  tcase_add_test(testcase, netaddr_cmp_test);
  tcase_add_test(testcase, netaddr_ncmp_test);
  tcase_add_test(testcase, netaddr_fnmatch_test);
  tcase_add_test(testcase, netaddr_get_sockaddr_test);
  tcase_add_test(testcase, netaddr_get_sockaddr_len_test);
  tcase_add_test(testcase, netaddr_set_sockaddr_test);
  tcase_add_test(testcase, netaddr_set_sockaddr_any_test);
  tcase_add_test(testcase, netaddr_get_inaddr_test);
  tcase_add_test(testcase, netaddr_get_inaddr_len_test);
  tcase_add_test(testcase, netaddr_get_port_test);
  tcase_add_test(testcase, netaddr_set_port_test);
  tcase_add_test(testcase, netaddr_set_reverse_dns_test);
  tcase_add_test(testcase, netaddr_get_dnsstr_test);
#ifdef PR_USE_IPV6
  tcase_add_test(testcase, netaddr_get_dnsstr_ipv6_test);
#endif /* PR_USE_IPV6 */
  tcase_add_test(testcase, netaddr_get_ipstr_test);
  tcase_add_test(testcase, netaddr_validate_dns_str_test);
  tcase_add_test(testcase, netaddr_get_localaddr_str_test);
  tcase_add_test(testcase, netaddr_is_v4_test);
  tcase_add_test(testcase, netaddr_is_v6_test);
  tcase_add_test(testcase, netaddr_is_v4mappedv6_test);
  tcase_add_test(testcase, netaddr_is_rfc1918_test);
  tcase_add_test(testcase, netaddr_disable_ipv6_test);
  tcase_add_test(testcase, netaddr_enable_ipv6_test);

  suite_add_tcase(suite, testcase);

  return suite;
}
