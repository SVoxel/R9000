/*
 * Copyright © 1999-2010 David Woodhouse <dwmw2@infradead.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/backing-dev.h>
#include <linux/compat.h>
#include <linux/mount.h>
#include <linux/blkpg.h>
#include <linux/magic.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/map.h>

#include <asm/uaccess.h>

#include "mtdcore.h"

int ath_nand_local_read(u_char *cal_part,loff_t from, size_t len,
                size_t *retlen, u_char *buf)
{
	struct mtd_info *mtd;

	mtd = get_mtd_device_nm((const char *)cal_part);
	if (IS_ERR_OR_NULL(mtd)) {
		printk("%s: %s not found\n", __func__, cal_part);
		return -ENXIO;
	}

	return mtd_read(mtd, from, len, retlen, buf);
}

EXPORT_SYMBOL(ath_nand_local_read);

