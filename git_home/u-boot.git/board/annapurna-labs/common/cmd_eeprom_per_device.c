 /*
   * board/annapurna-labs/common/cmd_eeprom_per_device.c
   *
   * U-Boot command for EEPROM per device operations
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

#include "eeprom_per_device.h"

static int do_eeprom_per_device_show(
	int			argc,
	char *const		argv[])
{
	uint8_t mac_addr[MAC_ADDR_LEN];
	int dirty;
	int i;

	for (i = 0; i < EEPROM_PER_DEVICE_NUM_MAC_ADDR; i++) {
		eeprom_per_device_mac_addr_get(i, mac_addr, &dirty);
		printf("MAC address %d: %02x:%02x:%02x:%02x:%02x:%02x%s\n",
			i, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
			mac_addr[4], mac_addr[5], dirty ? "(dirty)" : "");
	}

	return 0;
}

static int do_eeprom_per_device_mac_addr_set(
	int			argc,
	char *const		argv[])
{
	uint8_t mac_addr[MAC_ADDR_LEN];
	int idx;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	idx = simple_strtol(argv[1], NULL, 10);
	if ((idx < 0) || (idx >= EEPROM_PER_DEVICE_NUM_MAC_ADDR)) {
		printf("Syntax error - invalid MAC address index!\n\n");
		return -1;
	}

	eth_parse_enetaddr(argv[2], mac_addr);

	eeprom_per_device_mac_addr_set(idx, mac_addr);

	return 0;
}

static int do_eeprom_per_device_save(
	int			argc,
	char *const		argv[])
{
	if (argc != 1) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	if (eeprom_per_device_save()) {
		printf("Saving failed!\n\n");
		return -1;
	}

	return 0;
}

int do_eeprom_per_device(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	const char *op;

	if (argc < 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	op = argv[1];

	argv++;
	argc--;

	if (!strcmp(op, "show"))
		return do_eeprom_per_device_show(argc, argv);
	else if (!strcmp(op, "mac_addr_set"))
		return do_eeprom_per_device_mac_addr_set(argc, argv);
	else if (!strcmp(op, "save"))
		return do_eeprom_per_device_save(argc, argv);
	else {
		printf("Syntax error - invalid op!\n\n");
		return -1;
	}

	return 0;
}

U_BOOT_CMD(
	eeprom_per_device, 4, 0, do_eeprom_per_device,
	"EEPROM per device operations",
	"eeprom_per_device op <arg1> <arg2> ...\n\n"
	" - op - operation:\n"
	"        show - show all EEPROM per device information\n"
	"\n"
	"        mac_addr_set - Set MAC address\n"
	"             arg1 - index (0-3)\n"
	"             arg2 - MAC address (aa:bb:cc:dd:ee:ff)\n"
	"\n"
	"        save - Save all information to EEPROM\n"
	"\n\n"
);

