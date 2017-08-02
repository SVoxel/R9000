 /*
   * board/annapurna-labs/common/cmd_muio_mux.c
   *
   * Thie file contains a U-Boot command for controlling the multiplexing of
   * the chip's multi usage I/O pins.
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

#include <common.h>
#include <command.h>

#include "al_globals.h"

static struct {
	const enum al_muio_mux_if iface;
	const char *iface_str;
} ifaces_list[] = {
	{ AL_MUIO_MUX_IF_NOR_8,			"nor_8" },
	{ AL_MUIO_MUX_IF_NOR_16,		"nor_16" },
	{ AL_MUIO_MUX_IF_NOR_CS_0,		"nor_cs_0" },
	{ AL_MUIO_MUX_IF_NOR_CS_1,		"nor_cs_1" },
	{ AL_MUIO_MUX_IF_NOR_CS_2,		"nor_cs_2" },
	{ AL_MUIO_MUX_IF_NOR_CS_3,		"nor_cs_3" },
	{ AL_MUIO_MUX_IF_NOR_WP,		"nor_wp" },
	{ AL_MUIO_MUX_IF_NAND_8,		"nand_8" },
	{ AL_MUIO_MUX_IF_NAND_16,		"nand_16" },
	{ AL_MUIO_MUX_IF_NAND_CS_0,		"nand_cs_0" },
	{ AL_MUIO_MUX_IF_NAND_CS_1,		"nand_cs_1" },
	{ AL_MUIO_MUX_IF_NAND_CS_2,		"nand_cs_2" },
	{ AL_MUIO_MUX_IF_NAND_CS_3,		"nand_cs_3" },
	{ AL_MUIO_MUX_IF_NAND_WP,		"nand_wp" },
	{ AL_MUIO_MUX_IF_SRAM_8,		"sram_8" },
	{ AL_MUIO_MUX_IF_SRAM_16,		"sram_16" },
	{ AL_MUIO_MUX_IF_SRAM_CS_0,		"sram_cs_0" },
	{ AL_MUIO_MUX_IF_SRAM_CS_1,		"sram_cs_1" },
	{ AL_MUIO_MUX_IF_SRAM_CS_2,		"sram_cs_2" },
	{ AL_MUIO_MUX_IF_SRAM_CS_3,		"sram_cs_3" },
	{ AL_MUIO_MUX_IF_SATA_0_LEDS,		"sata_0_leds" },
	{ AL_MUIO_MUX_IF_SATA_1_LEDS,		"sata_1_leds" },
	{ AL_MUIO_MUX_IF_ETH_LEDS,		"eth_leds" },
	{ AL_MUIO_MUX_IF_ETH_GPIO,		"eth_gpio" },
	{ AL_MUIO_MUX_IF_UART_1,		"uart_1" },
	{ AL_MUIO_MUX_IF_UART_1_MODEM,		"uart_1_modem" },
	{ AL_MUIO_MUX_IF_UART_2,		"uart_2" },
	{ AL_MUIO_MUX_IF_UART_3,		"uart_3" },
	{ AL_MUIO_MUX_IF_I2C_GEN,		"i2c_gen" },
	{ AL_MUIO_MUX_IF_ULPI_0_RST_N,		"ulpi_0_rst_n" },
	{ AL_MUIO_MUX_IF_ULPI_1_RST_N,		"ulpi_1_rst_n" },
	{ AL_MUIO_MUX_IF_PCI_EP_INT_A,		"pci_ep_int_a" },
	{ AL_MUIO_MUX_IF_PCI_EP_RESET_OUT,	"pci_ep_reset_o" },
	{ AL_MUIO_MUX_IF_SPIM_A_SS_1,		"spim_a_ss_1" },
	{ AL_MUIO_MUX_IF_SPIM_A_SS_2,		"spim_a_ss_2" },
	{ AL_MUIO_MUX_IF_SPIM_A_SS_3,		"spim_a_ss_3" },
	{ AL_MUIO_MUX_IF_ULPI_1_B,		"ulpi_1_b" },
	{ AL_MUIO_MUX_IF_GPIO,			"gpio" },
};

static unsigned int ifaces_num = sizeof(ifaces_list) / sizeof(ifaces_list[0]);

int do_muio_mux(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err = 0;
	int i;

	const char *op;
	const char *iface_str;
	const char *iface_arg_str;

	enum al_muio_mux_if iface;
	int iface_arg;

	if (argc < 4) {
		printf("Syntax error!\n\n");
		return -1;
	}

	op = argv[1];
	iface_str = argv[2];
	iface_arg_str = argv[3];

	for (i = 0; i < ifaces_num; ++i) {
		if (!strcmp(ifaces_list[i].iface_str, iface_str)) {
			iface = ifaces_list[i].iface;
			break;
		}
	}

	if (i == ifaces_num) {
		printf("Syntax error - invalid iface!\n\n");
		return -1;
	}

	iface_arg = simple_strtol(iface_arg_str, NULL, 10);

	if (iface_arg < 0) {
		printf("Syntax error - invalid iface_arg!\n\n");
		return -1;
	}

	if (!strcmp(op, "alloc")) {
		err = al_muio_mux_iface_alloc(
			&al_globals.muio_mux,
			iface,
			iface_arg);
		if (err) {
			printf(
				"al_muio_mux_iface_alloc failed, either due to "
				"interface conflict, or syntax error!\n\n");
			return -1;

		}
	} else if (!strcmp(op, "free")) {
		err = al_muio_mux_iface_free(
			&al_globals.muio_mux,
			iface,
			iface_arg);
		if (err) {
			printf(
				"al_muio_mux_iface_free failed, probably due to "
				"syntax error!\n\n");
			return -1;

		}
	} else if (!strcmp(op, "falloc")) {
		err = al_muio_mux_iface_alloc_force(
			&al_globals.muio_mux,
			iface,
			iface_arg);
		if (err) {
			printf(
				"al_muio_mux_iface_alloc_force failed, probably "
				"due to syntax error!\n\n");
			return -1;

		}
	} else {
		printf("Syntax error - invalid op!\n\n");
		return -1;
	}

	return 0;
}

U_BOOT_CMD(
	muio_mux, 4, 1, do_muio_mux,
	"Multi usage I/O pins (MUIO) multiplexing control - debug",
	"muio_mux op iface iface_arg\n\n"
	" - op - operation:\n"
	"        alloc - allocate an interface\n"
	"        free - free an interface\n"
	"        falloc - force allocate an interface\n"
	" - iface - the interface to operate on:\n"
	"        nor_8, nor_16\n"
	"        nor_cs_0, nor_cs_1, nor_cs_2, nor_cs_3, nor_wp\n"
	"        nand_8, nand_16\n"
	"        nand_cs_0, nand_cs_1, nand_cs_2, nand_cs_3, nand_wp\n"
	"        sram_8, sram_16, sram_cs_0, sram_cs_1, sram_cs_2, sram_cs_3\n"
	"        sata_0_leds, sata_1_leds, eth_leds\n"
	"        eth_gpio\n"
	"        uart_1, uart_1_modem, uart_2, uart_3\n"
	"        i2c_gen\n"
	"        ulpi_0_rst_n, ulpi_1_rst_n\n"
	"        pci_ep_int_a, pci_ep_reset_o\n"
	"        spim_a_ss_1, spim_a_ss_2, spim_a_ss_3\n"
	"        ulpi_1_b\n"
	"        gpio\n"
	" - iface_arg - interface specific argument:\n"
	"        gpio - gpio index (decimal: 0 - 43)\n\n");

