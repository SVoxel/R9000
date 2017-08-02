/*
 * ProFTPD - mod_sftp message format
 * Copyright (c) 2008-2013 TJ Saunders
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
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 *
 * $Id: msg.h,v 1.7 2013-03-28 19:56:13 castaglia Exp $
 */

#include "mod_sftp.h"

#ifndef MOD_SFTP_MSG_H
#define MOD_SFTP_MSG_H

char sftp_msg_read_byte(pool *, unsigned char **, uint32_t *);
int sftp_msg_read_bool(pool *, unsigned char **, uint32_t *);
unsigned char *sftp_msg_read_data(pool *, unsigned char **, uint32_t *, size_t);
#ifdef PR_USE_OPENSSL_ECC
EC_POINT *sftp_msg_read_ecpoint(pool *, unsigned char **, uint32_t *,
  const EC_GROUP *, EC_POINT *);
#endif /* PR_USE_OPENSSL_ECC */
uint32_t sftp_msg_read_int(pool *, unsigned char **, uint32_t *);
uint64_t sftp_msg_read_long(pool *, unsigned char **, uint32_t *);
BIGNUM *sftp_msg_read_mpint(pool *, unsigned char **, uint32_t *);
char *sftp_msg_read_string(pool *, unsigned char **, uint32_t *);

uint32_t sftp_msg_write_byte(unsigned char **, uint32_t *, char);
uint32_t sftp_msg_write_bool(unsigned char **, uint32_t *, char);
uint32_t sftp_msg_write_data(unsigned char **, uint32_t *,
  const unsigned char *, size_t, int);
#ifdef PR_USE_OPENSSL_ECC
uint32_t sftp_msg_write_ecpoint(unsigned char **, uint32_t *, const EC_GROUP *,
  const EC_POINT *);
#endif /* PR_USE_OPENSSL_ECC */
uint32_t sftp_msg_write_int(unsigned char **, uint32_t *, uint32_t);
uint32_t sftp_msg_write_long(unsigned char **, uint32_t *, uint64_t);
uint32_t sftp_msg_write_mpint(unsigned char **, uint32_t *, const BIGNUM *);
uint32_t sftp_msg_write_string(unsigned char **, uint32_t *, const char *);

/* Utility method for obtaining a scratch buffer for constructing SSH2
 * messages without necessarily needing an SSH2 packet.
 */
unsigned char *sftp_msg_getbuf(pool *, size_t);

#endif
