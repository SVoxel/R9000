/**
 * board/annapurna-labs/common/iodma.h
 *
 * Thie file contains a RAID and memory services accelerator services
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
#ifndef __IODMA_H__
#define __IODMA_H__

#include <common.h>

#include "al_hal_plat_types.h"

int iodma_init(void);

void iodma_terminate(void);

int iodma_memcpy(
	al_phys_addr_t	dst,
	al_phys_addr_t	src,
	uint64_t	size);

int iodma_memset(
	al_phys_addr_t	dst,
	uint8_t		val,
	uint64_t	size);

#endif

