 /*
   * board/annapurna-labs/common/cmd_cpu_misc.c
   *
   * Thie file contains a U-Boot command for setting CPU speed
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
#include <command.h>

#include "al_globals.h"
#include "al_hal_pll.h"

int do_cpu_set_speed(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	enum al_pll_freq pll_freq;
	unsigned int pll_freq_val;
	struct al_pll_obj obj;
	unsigned int speed_khz;
	int err = 0;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	speed_khz = 1000 * simple_strtol(argv[1], NULL, 10);

	/* Init SB PLL object */
	err = al_pll_init(
		(void __iomem *)AL_PLL_BASE(AL_PLL_CPU),
		"CPU PLL",
		(al_globals.bootstraps.pll_ref_clk_freq == 25000000) ?
		AL_PLL_REF_CLK_FREQ_25_MHZ :
		AL_PLL_REF_CLK_FREQ_125_MHZ,
		&obj);
	if (err) {
		printf("%s: al_pll_init failed!\n", __func__);
		return -1;
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

	/*
	 * Check if the required frequency can be derived from the
	 * PLL frequency
	 */
	if (pll_freq_val % speed_khz) {
		printf("%s: PLL freq (%u) not suitable for %dMHz channel!\n",
			__func__, pll_freq_val, speed_khz / 1000);
		return -1;
	}

	/* Set the channel's PLL divider */
	err = al_pll_channel_div_set(
		&obj,
		0,
		pll_freq_val / speed_khz,
		0,
		0,	/* No reset */
		1,	/* Apply on last channel */
		1000);	/* 1ms timeout to settle */
	if (err) {
		printf("%s: al_pll_channel_div_set failed!\n",
				__func__);
		return -1;
	}

	return 0;
}

#ifndef CONFIG_ARM64
extern volatile unsigned int data_abort_is_slient;
extern volatile unsigned int num_data_aborts;

int abort_is_pending(void)
{
	unsigned int reg;

	asm volatile ("mrc p15, 0, %0, c12, c1, 0\n" : "=r" (reg));

	return !!(reg & (1 << 8));
}

void aborts_enable(void)
{
	unsigned int reg;

	asm volatile ("mrs %0, cpsr\n" : "=r" (reg));
	asm volatile ("bic %0, #(1 << 8)\n" : "=r" (reg));
	asm volatile ("msr cpsr_x, %0\n" : "=r" (reg));
}

void aborts_disable(void)
{
	unsigned int reg;

	asm volatile ("mrs %0, cpsr\n" : "=r" (reg));
	asm volatile ("orr %0, #(1 << 8)\n" : "=r" (reg));
	asm volatile ("msr cpsr_x, %0\n" : "=r" (reg));
}

void soak_all_aborts(void)
{
	data_abort_is_slient = 1;
	mb();
	aborts_enable();
	mb();
	while (abort_is_pending());
	mb();
	aborts_disable();
	mb();
	data_abort_is_slient = 0;
	num_data_aborts = 0;
}

int do_cpu_aborts_enable_set(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	unsigned int en;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	en = simple_strtol(argv[1], NULL, 10);

	if (en)
		aborts_enable();
	else
		aborts_disable();

	return 0;
}
#else
unsigned int data_abort_is_slient;
unsigned int num_data_aborts;

int abort_is_pending(void)
{
	return 0;
}

void aborts_enable(void)
{
}

void aborts_disable(void)
{
}

void soak_all_aborts(void)
{
}
#endif

U_BOOT_CMD(
	cpu_set_speed, 2, 0, do_cpu_set_speed,
	"Set CPU speed",
	"cpu_set_speed <speed - MHz>\n\n");

#ifndef CONFIG_ARM64
U_BOOT_CMD(
	cpu_aborts_enable_set, 2, 0, do_cpu_aborts_enable_set,
	"Set enabling state of CPU aborts",
	"cpu_aborts_enable_set <0 - disable, 1 - enable>\n\n");
#endif


