/*
 * fs/btrfs/csum.h
 *
 * Btrfs checksum utils - header file
 *
 * Copyright (C) 2013 Annapurna Labs Ltd.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CSUM__
#define __CSUM__

#include <linux/init.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include "ctree.h"

/* calculates a digest for the given page and writes the checksum to result */
void btrfs_csum_page_digest(
	struct page	*page,
	unsigned int	offset,
	size_t		len,
	u32		*result);

/* calculates a digest for multiple pages in parallel.
 * note that the checksum is calculated for each page separately!
 * this api is useful for submitting multiple pages for checksum calculation
 * and waiting for them all to finish by calling final()
 */
void *btrfs_csum_mpage_init(unsigned int pages_n);
void btrfs_csum_mpage_digest(void *mpage_priv,
			     struct page *page, unsigned int offset,
			     size_t len, u32 *result);
void btrfs_csum_mpage_final(void *mpage_priv);

int btrfs_csum_init(void);
void btrfs_csum_exit(void);

#endif
