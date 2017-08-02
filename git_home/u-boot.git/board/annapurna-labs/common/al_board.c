/**
 * board/annapurna-labs/common/al_board.c
 *
 * Thie file contains Annapurna Labs common board functionality.
 *
 * Copyright (C) 2012 Annapurna Labs Ltd.
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
#include <errno.h>
#include <common.h>
#include <linux/compiler.h>
#include <asm/io.h>
#include <asm/global_data.h>

#include "al_globals.h"
#include "al_board.h"
#include "al_hal_watchdog_regs.h"
#include "al_hal_pll.h"
#include <spi.h>

DECLARE_GLOBAL_DATA_PTR;

int bootstrap_read_and_parse(struct al_bootstrap *b_ptr)
{
	enum al_pll_freq pll_freq;
	unsigned int pll_freq_val;
	unsigned int chan_freq;
	struct al_pll_obj obj;
	int err;

	err = al_bootstrap_parse((void *)AL_PBS_REGFILE_BASE, b_ptr);
	if (err)
		return err;

	err = al_pll_init(
		(void __iomem *)AL_PLL_BASE(AL_PLL_NB),
		"NB PLL",
		(b_ptr->pll_ref_clk_freq == 25000000) ?
		AL_PLL_REF_CLK_FREQ_25_MHZ :
		AL_PLL_REF_CLK_FREQ_100_MHZ,
		&obj);
	if (err)
		return err;

	err = al_pll_freq_get(
		&obj,
		&pll_freq,
		&pll_freq_val);
	if (err)
		return err;

	err = al_pll_channel_freq_get(&obj, 0, &chan_freq);
	if (err)
		return err;

	b_ptr->ddr_pll_freq = chan_freq * 1000;

	return 0;
}

int bootstrap_get_aux(struct al_bootstrap *b_ptr)
{
	int err = 0;

	if (gd->flags & GD_FLG_DEVINIT)
		*b_ptr = al_globals.bootstraps;
	else
		err = bootstrap_read_and_parse(b_ptr);

	return err;
}

int bootstrap_get(struct al_bootstrap *b_ptr)
{
	return bootstrap_get_aux(b_ptr);
}

unsigned int al_bootstrap_sb_clk_get(
	void)
{
	int err;
	unsigned int val = 1;
	struct al_bootstrap bootstrap;

	err = bootstrap_get_aux(&bootstrap);
	if (!err)
		val = bootstrap.sb_clk_freq;

	return val;
}

unsigned int al_bootstrap_nb_clk_get(
	void)
{
	int err;
	unsigned int val = 1;
	struct al_bootstrap bootstrap;

	err = bootstrap_get_aux(&bootstrap);
	if (!err)
		val = bootstrap.ddr_pll_freq;

	return val;
}

unsigned int al_spi_mode_get(void)
{
	int err;
	unsigned int val = 0;
	struct al_bootstrap bootstrap;

	err = bootstrap_get_aux(&bootstrap);
	if (!err)
		val = (bootstrap.boot_device ==	BOOT_DEVICE_SPI_MODE_3) ?
				SPI_MODE_3 : SPI_MODE_0;

	return val;
}

#ifdef CONFIG_AL_SPI
static unsigned int al_spi_baud_rate;

unsigned int al_spi_baud_rate_get(void)
{
	if (!al_spi_baud_rate) {
		printf("%s: called too early!\n", __func__);
		return AL_SPI_BAUD_RATE_DEFAULT;
	}

	return al_spi_baud_rate;
}

void al_spi_baud_rate_set(unsigned int baud_rate)
{
	debug("%s(%u)\n", __func__, baud_rate);
	al_spi_baud_rate = baud_rate;
}
#endif

void reset_cpu(ulong addr)
{
	struct al_watchdog_regs __iomem *watchdog_regs =
		(struct al_watchdog_regs __iomem *)AL_WD_BASE(0);

	al_reg_write32(&watchdog_regs->load, 0);
	al_reg_write32(&watchdog_regs->control,
		AL_WATCHDOG_CTRL_INT_ENABLE | AL_WATCHDOG_CTRL_RESET_ENABLE);

	while (1)
		;
}
