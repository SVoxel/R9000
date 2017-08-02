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
#include <al_hal_iomap.h>

#include "shared_params.h"
#include "al_hal_common.h"

static inline struct shared_parameters *_shared_params_ptr_get(void)
{
	struct shared_parameters *shared_params = (struct shared_parameters *)
		(AL_PBS_INT_MEM_SRAM_BASE + PBS_INT_MEM_SHARED_PARAMS_OFFSET);

	return shared_params;
}

int shared_params_valid()
{
	struct shared_parameters *shared_params = _shared_params_ptr_get();

	return (shared_params->magic_num == SHARED_PARAMS_MN);
}

struct shared_parameters *shared_params_ptr_get()
{
	return _shared_params_ptr_get();
}

