/**
 * board/annapurna-labs/common/pll_init.c
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
#include <common.h>
#include <asm/io.h>
#include <asm/global_data.h>

#include <al_globals.h>
#include <pll_init.h>

DECLARE_GLOBAL_DATA_PTR;

/* SB PLL Initialization */
int pll_sb_init(
	const struct pll_cfg_ent	*cfg,
	int				cnt)
{
	int err = 0;

	struct al_pll_obj obj;

	enum al_pll_freq pll_freq;
	unsigned int pll_freq_val;

	/* Init SB PLL object */
	err = al_pll_init(
		(void __iomem *)AL_PLL_BASE(AL_PLL_SB),
		"SB PLL",
		(al_globals.bootstraps.pll_ref_clk_freq == 25000000) ?
		AL_PLL_REF_CLK_FREQ_25_MHZ :
		AL_PLL_REF_CLK_FREQ_100_MHZ,
		&obj);
	if (err) {
		printf("%s: al_pll_init failed!\n", __func__);
		return err;
	}

	/* Obtain PLL current frequency */
	err = al_pll_freq_get(
		&obj,
		&pll_freq,
		&pll_freq_val);
	if (err) {
		printf("%s: al_pll_freq_get failed!\n", __func__);
		return err;
	}

	for (; cnt > 0; cnt--, cfg++) {
		/**
		 * Check if the required frequency can be derived from the
		 * PLL frequency
		 */
		if (pll_freq_val % cfg->freq_khz) {
			printf("%s: PLL freq not suitable for %dMHz channel!\n",
				__func__, cfg->freq_khz / 1000);
			return -EINVAL;
		}

		/* Set the channel's PLL divider */
		err = al_pll_channel_div_set(
			&obj,
			cfg->chan_idx,
			pll_freq_val / cfg->freq_khz,
			0,
			0,			/* No reset */
			(cnt > 1) ? 0 : 1,	/* Apply on last channel */
			(cnt > 1) ? 0 : 1000);	/* 1ms timeout to settle */
		if (err) {
			printf("%s: al_pll_channel_div_set failed!\n",
					__func__);
			return err;
		}
	}

	return 0;
}
