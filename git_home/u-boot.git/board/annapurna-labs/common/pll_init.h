/**
 * board/annapurna-labs/common/pll_init.h
 *
 * PLL initialization service
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

#include "al_hal_pll.h"

/* PLL configuration entry */
struct pll_cfg_ent {
	/* PLL channel index */
	unsigned int chan_idx;

	/* Required channel frequency [KHz] */
	unsigned int freq_khz;
};

/**
 * SB PLL Initialization
 *
 * @param[in]  cfg
 *             PLL configuration entry array
 *
 * @param[in]  cnt
 *             Number of PLL configuration entries
 *
 * @return 0 upon success
 */
int pll_sb_init(
	const struct pll_cfg_ent	*cfg,
	int				cnt);
