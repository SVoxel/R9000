/*
 * ProFTPD: mod_log
 *
 * Copyright (c) 2013 TJ Saunders
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
 * $Id: mod_log.h,v 1.2 2013-11-11 01:34:04 castaglia Exp $
 */

#ifndef MOD_LOG_H
#define MOD_LOG_H

/* These "meta" sequences represent the parsed LogFormat variables. */
#define LOGFMT_META_START		0xff
#define LOGFMT_META_ARG_END		0xfe
#define LOGFMT_META_ARG			1
#define LOGFMT_META_BYTES_SENT		2
#define LOGFMT_META_FILENAME		3
#define LOGFMT_META_ENV_VAR		4
#define LOGFMT_META_REMOTE_HOST		5
#define LOGFMT_META_REMOTE_IP		6
#define LOGFMT_META_IDENT_USER		7
#define LOGFMT_META_PID			8
#define LOGFMT_META_TIME		9
#define LOGFMT_META_SECONDS		10
#define LOGFMT_META_COMMAND		11
#define LOGFMT_META_LOCAL_NAME		12
#define LOGFMT_META_LOCAL_PORT		13
#define LOGFMT_META_LOCAL_IP		14
#define LOGFMT_META_LOCAL_FQDN		15
#define LOGFMT_META_USER		16
#define LOGFMT_META_ORIGINAL_USER	17
#define LOGFMT_META_RESPONSE_CODE	18
#define LOGFMT_META_CLASS		19
#define LOGFMT_META_ANON_PASS		20
#define LOGFMT_META_METHOD		21
#define LOGFMT_META_XFER_PATH		22
#define LOGFMT_META_DIR_NAME		23
#define LOGFMT_META_DIR_PATH		24
#define LOGFMT_META_CMD_PARAMS		25
#define LOGFMT_META_RESPONSE_STR	26
#define LOGFMT_META_PROTOCOL		27
#define LOGFMT_META_VERSION		28
#define LOGFMT_META_RENAME_FROM		29
#define LOGFMT_META_FILE_MODIFIED	30
#define LOGFMT_META_UID			31
#define LOGFMT_META_GID			32
#define LOGFMT_META_RAW_BYTES_IN	33
#define LOGFMT_META_RAW_BYTES_OUT	34
#define LOGFMT_META_EOS_REASON		35
#define LOGFMT_META_VHOST_IP		36
#define LOGFMT_META_NOTE_VAR		37
#define LOGFMT_META_XFER_STATUS		38
#define LOGFMT_META_XFER_FAILURE	39
#define LOGFMT_META_MICROSECS		40
#define LOGFMT_META_MILLISECS		41
#define LOGFMT_META_ISO8601		42
#define LOGFMT_META_GROUP		43
#define LOGFMT_META_BASENAME		44

#endif /* MOD_LOG_H */
