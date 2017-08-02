/*
 * Copyright(c) 2013 Annapurna Labs.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */
/**
 * This file includes shared parameters between the stage2 and the uboot
 */
#ifndef __SHARED_PARAMS_H__
#define __SHARED_PARAMS_H__

#include <linux/types.h>

/* The expected magic number for validating the shared params */
#define SHARED_PARAMS_MN	0x31415926

struct shared_parameters {
	uint32_t magic_num; /* Magic number for validating the shared params */
	uint64_t ddr_size; /* DDR total size in bytes */
};

/*
 * Shared params validity check
 * Returns non zero if the shared parameters are valid
 */
int shared_params_valid(void);

/*
 * return pointer to shared parameters between stage2 and u-boot
 * any parameter that need to be shared should be added to shared_parameters
 */
struct shared_parameters *shared_params_ptr_get(void);

#endif
