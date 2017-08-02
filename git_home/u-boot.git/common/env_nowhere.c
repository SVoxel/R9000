/*
 * (C) Copyright 2000-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
env_t *env_ptr;
#endif

#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
void env_nowhere_relocate_spec(void)
{
}
#endif
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
void env_relocate_spec(void)
{
}
#endif

/*
 * Initialize Environment use
 *
 * We are still running from ROM, so data use is limited
 */
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
int env_nowhere_init(void)
{
	gd->env_addr	= (ulong)&default_environment[0];
	gd->env_valid	= 0;

	return 0;
}
#endif
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
int env_init(void)
{
	gd->env_addr    = (ulong)&default_environment[0];
	gd->env_valid   = 0;

	return 0;
}
#endif
