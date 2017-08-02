 /*
   * board/annapurna-labs/common/cmd_al_pcie_ext.c
   *
   * Thie file contains a U-Boot command for enabling AL PCIe external
   *
   * Copyright (C) 2014 Annapurna Labs Ltd.
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

extern void pci_init_board_external();

int do_al_pcie_ext(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	pci_init_board_external();
}

U_BOOT_CMD(
	al_pcie_ext, 1, 1, do_al_pcie_ext,
	"Enable AL PCIe external",
	"");

