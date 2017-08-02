 /*
   * board/annapurna-labs/common/cmd_wfi.c
   *
   * Thie file contains a U-Boot command for waiting for interrupt (CPU idle)
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
#include <asm/io.h>

int do_wfi(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	wfi();

	return 0;
}

U_BOOT_CMD(
	wfi, 1, 1, do_wfi,
	"CPU wait for interrupt CPU idle)",
	"");

