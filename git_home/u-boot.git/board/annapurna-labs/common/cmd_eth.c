 /*
   * board/annapurna-labs/common/cmd_eth.c
   *
   * Thie file contains a U-Boot commands for setting ethernet configurations
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

#include "al_globals.h"
#include "al_eth.h"

int do_eth_freeze_serdes_settings(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	al_bool	enable = 0;
	unsigned int port_num;
	struct al_eth_board_params params;
	int err = 0;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	port_num = simple_strtol(argv[1], NULL, 10);
	if (port_num < 0 || port_num > 3) {
		printf("Syntax error - invalid port number!\n\n");
		return -1;
	}

	if (!strcmp(argv[2], "enable")) {
		enable = AL_TRUE;
	} else if (!strcmp(argv[2], "disable")) {
		enable = AL_FALSE;
	} else {
		printf("Syntax error - invalid parameter\n");
		return -1;
	}

	err = al_eth_board_params_load(port_num, &params);
	if (err != 0) {
		printf("board params load return error\n");
		return -1;
	}

	params.dont_override_serdes = enable;

	if (enable) {
		if (params.media_type == AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED)
			printf("WARNING: freeze settings won't take effect in auto-speed mode.\n"
			       "use eth_mac_mode_set to change it.\n");
		if (params.kr_lt_enable == AL_TRUE)
			printf("WARNING: link training is enabled and may "
			       "change the serdes settings.\n"
			       "use eth_link_training_enable to disable it.\n");
	}

	err = al_eth_board_params_store(port_num, &params);
	if (err != 0) {
		printf("board params store return error\n");
		return -1;
	}

	err = al_eth_serdes_override_set(port_num,
				(enable == AL_TRUE) ? AL_FALSE : AL_TRUE);
	if (err != 0) {
		printf("serdes_override_set return error\n");
		return -1;
	}

	return 0;
}

int do_eth_1g_params_set(cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err = 0;
	unsigned int port_num;
	struct al_eth_board_params params;
	al_bool autoneg;
	al_bool sgmii;
	uint32_t speed = 1000;
	al_bool full_duplex = AL_TRUE;

	if (argc < 4) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	port_num = simple_strtol(argv[1], NULL, 10);
	if (port_num < 0 || port_num > 3) {
		printf("Syntax error - invalid port number!\n\n");
		return -1;
	}

	if (!strcmp(argv[2], "sgmii")) {
		sgmii = AL_TRUE;
	} else if (!strcmp(argv[2], "1000basex")) {
		sgmii = AL_FALSE;
	} else {
		printf("Syntax error - invalid parameter\n");
		return -1;
	}

	if (!strcmp(argv[3], "enable")) {
		autoneg = AL_TRUE;
	} else if (!strcmp(argv[3], "disable")) {
		autoneg = AL_FALSE;
	} else {
		printf("Syntax error - invalid parameter\n");
		return -1;
	}

	if (autoneg == AL_FALSE) {
		if (argc < 6) {
			printf("Syntax error - invalid number of parameters!\n\n");
			return -1;
		}

		speed = simple_strtol(argv[4], NULL, 10);
		if ((speed != 1000) && (speed != 100) && (speed != 10)) {
			printf("Syntax error - invalid speed parameter\n");
			return -1;
		}

		if (!strcmp(argv[5], "full")) {
			full_duplex = AL_TRUE;
		} else if (!strcmp(argv[5], "half")) {
			full_duplex = AL_FALSE;
		} else {
			printf("Syntax error - invalid duplex parameter\n");
			return -1;
		}

		if ((speed == 1000) && (full_duplex == AL_FALSE)) {
			printf("half duplex in 1Gbps is not supported.\n");
			return -1;
		}
	}

	err = al_eth_board_params_load(port_num, &params);
	if (err != 0) {
		printf("board params load return error\n");
		return -1;
	}

	params.an_disable = (autoneg == AL_TRUE) ? AL_FALSE : AL_TRUE;
	params.force_1000_base_x = (sgmii == AL_TRUE) ? AL_FALSE : AL_TRUE;
	params.half_duplex = (full_duplex == AL_TRUE) ? AL_FALSE : AL_TRUE;
	if (speed == 1000)
		params.speed = AL_ETH_BOARD_1G_SPEED_1000M;
	else if (speed == 100)
		params.speed = AL_ETH_BOARD_1G_SPEED_100M;
	else
		params.speed = AL_ETH_BOARD_1G_SPEED_10M;

	err = al_eth_board_params_store(port_num, &params);
	if (err != 0) {
		printf("board params store return error\n");
		return -1;
	}

	err = al_eth_mac_link_set(port_num, sgmii, autoneg, speed, full_duplex);

	return err;
}

int do_eth_mac_mode_set(cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err = 0;
	unsigned int port_num;
	struct al_eth_board_params params;
	enum al_eth_board_media_type mac_mode;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	port_num = simple_strtol(argv[1], NULL, 10);
	if (port_num < 0 || port_num > 3) {
		printf("Syntax error - invalid port number!\n\n");
		return -1;
	}

	mac_mode = simple_strtol(argv[2], NULL, 10);

	err = al_eth_board_params_load(port_num, &params);
	if (err != 0) {
		printf("board params load return error\n");
		return -1;
	}

	params.media_type = mac_mode;

	err = al_eth_board_params_store(port_num, &params);
	if (err != 0) {
		printf("board params store return error\n");
		return -1;
	}

	err = al_eth_mac_mode_set(port_num, mac_mode);

	return err;
}

int do_eth_link_training_enable(cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err = 0;
	unsigned int port_num;
	struct al_eth_board_params params;
	al_bool lt_enable;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	port_num = simple_strtol(argv[1], NULL, 10);
	if (port_num < 0 || port_num > 3) {
		printf("Syntax error - invalid port number!\n\n");
		return -1;
	}

	if (!strcmp(argv[2], "enable")) {
		lt_enable = AL_TRUE;
	} else if (!strcmp(argv[2], "disable")) {
		lt_enable = AL_FALSE;
	} else {
		printf("Syntax error - invalid parameter\n");
		return -1;
	}

	err = al_eth_board_params_load(port_num, &params);
	if (err != 0) {
		printf("board params load return error\n");
		return -1;
	}

	params.autoneg_enable = lt_enable;
	params.kr_lt_enable = lt_enable;

	err = al_eth_board_params_store(port_num, &params);
	if (err != 0) {
		printf("board params store return error\n");
		return -1;
	}

	err = al_eth_link_training_enable(port_num, lt_enable);

	return err;
}

int do_eth_lm_debug_enable(cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err = 0;
	unsigned int port_num;
	al_bool enable;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	port_num = simple_strtol(argv[1], NULL, 10);
	if (port_num < 0 || port_num > 3) {
		printf("Syntax error - invalid port number!\n\n");
		return -1;
	}

	if (!strcmp(argv[2], "enable")) {
		enable = AL_TRUE;
	} else if (!strcmp(argv[2], "disable")) {
		enable = AL_FALSE;
	} else {
		printf("Syntax error - invalid parameter\n");
		return -1;
	}

	err = al_eth_link_management_debug_enable(port_num, enable);

	return err;
}

int do_eth_retimer_config(cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	char				*cmd;
	int				err = 0;
	unsigned int			port_num;
	struct al_eth_board_params	params;
	int				retimer_exist = -1;
	int				retimer_bus_id = -1;
	int				retimer_i2c_addr = -1;
	int				retimer_channel = -1;

	if (argc < 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	argv++;
	argc--;

	port_num = simple_strtol(argv[0], NULL, 10);
	if (port_num < 0 || port_num > 3) {
		printf("Syntax error - invalid port number!\n\n");
		return -1;
	}

	argv++;
	argc--;

	while (argc > 0) {
		cmd = argv[0];

		if (!strcmp(cmd, "--exist")) {
			if (argc < 1) {
				printf("Syntax error - exist should get value!\n\n");
				return -1;
			}

			argv++;
			argc--;

			if (!strncmp("0", argv[0], 10))
				retimer_exist = 0;
			else if (!strncmp("1", argv[0], 10))
				retimer_exist = 1;
			else {
				printf("Syntax error - exist can get only 0 or 1!\n\n");
				return -1;
			}
		} else if (!strcmp(cmd, "--bus-id")) {
			if (argc < 1) {
				printf("Syntax error - bus id should get value!\n\n");
				return -1;
			}

			argv++;
			argc--;

			retimer_bus_id = simple_strtol(argv[0], NULL, 10);
		} else if (!strcmp(cmd, "--i2c-addr")) {
			if (argc < 1) {
				printf("Syntax error - i2c-addr should get value!\n\n");
				return -1;
			}

			argv++;
			argc--;

			retimer_i2c_addr = simple_strtol(argv[0], NULL, 10);
		} else if (!strcmp(cmd, "--channel")) {
			if (argc < 1) {
				printf("Syntax error - channel should get value!\n\n");
				return -1;
			}

			argv++;
			argc--;

			if (!strncmp("A", argv[0], 10))
				retimer_exist = 0;
			else if (!strncmp("B", argv[0], 10))
				retimer_exist = 1;
			else {
				printf("Syntax error - channel can get only A or B!\n\n");
				return -1;
			}
		} else {
			printf("Syntax error - unknown option!\n\n");
			return -1;
		}


		argv++;
		argc--;
	}

	err = al_eth_board_params_load(port_num, &params);
	if (err != 0) {
		printf("board params load return error\n");
		return -1;
	}

	if (retimer_exist != -1)
		params.retimer_exist = retimer_exist;
	else
		retimer_exist = params.retimer_exist;

	if (retimer_bus_id != -1)
		params.retimer_bus_id = retimer_bus_id;
	else
		retimer_bus_id = params.retimer_bus_id;

	if (retimer_i2c_addr != -1)
		params.retimer_i2c_addr = retimer_i2c_addr;
	else
		retimer_i2c_addr = params.retimer_i2c_addr;

	if (retimer_channel != -1)
		params.retimer_channel = retimer_channel;
	else
		retimer_channel = params.retimer_channel;

	err = al_eth_board_params_store(port_num, &params);
	if (err != 0) {
		printf("board params store return error\n");
		return -1;
	}

	err = al_eth_retimer_config_override(port_num, retimer_exist,
					     retimer_bus_id, retimer_i2c_addr,
					     retimer_channel);

	return err;
}



U_BOOT_CMD(
	eth_freeze_serdes_settings, 3, 0, do_eth_freeze_serdes_settings,
	"freeze serdes parameters to be used in the upper layer (disable by default)\n"
	"* enabling it will cause the upper layer to avoid replacing these parameters\n"
	"* enabled mode should be used in case the serdes parameter were set in the u-boot\n",
	"eth_freeze_serdes_settings <port> <enable/disable>\n\n");

U_BOOT_CMD(
	eth_1g_params_set, 6, 0, do_eth_1g_params_set,
	"configure 1G link parameters."
	"these parameters will also be applied in Linux\n",
	"eth_1g_params_set <port> <type> <autoneg> <[speed]> <[duplex]>\n"
	" - port - port number\n"
	" - type - 1000basex / sgmii\n"
	" - autoneg - enable / disable\n"
	"	if autoneg is disabled the following parameters is required\n"
	"	- speed - 10/100/1000\n"
	"	- duplex - full / half\n\n");

U_BOOT_CMD(
	eth_mac_mode_set, 3, 0, do_eth_mac_mode_set,
	"override the mac mode from the device tree\n",
	"eth_mac_mode_set <port> <mac mode>\n"
	" - port - port number\n"
	" - mac mode:\n"
	"	0 - Auto-detect\n"
	"	1 - RGMII\n"
	"	2 - 10G serial\n"
	"	3 - SGMII\n"
	"	4 - 1000BASE-X\n"
	"	5 - Auto-detect Auto-speed\n");

U_BOOT_CMD(
	eth_link_training_enable, 3, 0, do_eth_link_training_enable,
	"enable / disable link training\n",
	"eth_link_training_enable <port> <enable/disable>\n\n");

U_BOOT_CMD(
	eth_lm_debug_enable, 3, 0, do_eth_lm_debug_enable,
	"enable / disable link management debug enable\n",
	"eth_lm_debug_enable <port> <enable/disable>\n\n");

U_BOOT_CMD(
	eth_retimer_config, 6, 0, do_eth_retimer_config,
	"override retimer configuration\n",
	"eth_retimer_config <port> [options]\n"
	"options:\n"
	"--exist	<1/0>	retimer is exist on the board\n"
	"--bus-id	<bus>	i2c bus connected to the retimer\n"
	"--i2c-addr	<addr>	i2c address of the retimer\n"
	"--channel	<A/B>	what channel to use for this port\n\n");