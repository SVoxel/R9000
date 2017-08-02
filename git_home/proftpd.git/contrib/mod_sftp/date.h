/*
 * ProFTPD - mod_sftp date(1) simulation
 * Copyright (c) 2012 TJ Saunders
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 *
 * $Id: date.h,v 1.2 2012-02-15 23:50:51 castaglia Exp $
 */

#include "mod_sftp.h"

#ifndef MOD_SFTP_DATE_H
#define MOD_SFTP_DATE_H

int sftp_date_handle_packet(pool *, void *, uint32_t, unsigned char *,
  uint32_t);

int sftp_date_postopen_session(uint32_t);
int sftp_date_open_session(uint32_t);
int sftp_date_close_session(uint32_t);

/* Prepare for the SCP transfer, setting all the options and paths that will
 * be involved.
 */
int sftp_date_set_params(pool *, uint32_t, array_header *);

#endif
