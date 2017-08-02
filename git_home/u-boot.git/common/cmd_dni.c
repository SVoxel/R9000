/*
 * (C) Copyright 2001
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * DNI Utilities
 */

#include <common.h>
#include <command.h>
#include <miiphy.h>
#ifdef DNI_NAND
#include <nand.h>
#endif
#include <errno.h>
#include <dni_common.h>

#ifdef BOARDCAL

void get_board_data(int offset, int len, u8* buf)
{
#ifdef DNI_NAND
	size_t read_size = CONFIG_SYS_FLASH_SECTOR_SIZE;
	unsigned char buffer[CONFIG_SYS_FLASH_SECTOR_SIZE];
	int ret = 0;
#endif

#ifdef DNI_NAND
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	ret = nand_read_skip_bad_old(&nand_info[0], BOARDCAL, &read_size, (u_char *)buffer);
#endif
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ret = nand_read_skip_bad(&nand_info[0], BOARDCAL, &read_size, NULL, CONFIG_SYS_FLASH_SECTOR_SIZE, (u_char *)buffer);
#endif
	printf(" %zu bytes read: %s\n", read_size,
	       ret ? "ERROR" : "OK");
#endif

#ifdef DNI_NAND
	memcpy(buf, (void *)(buffer + offset), len);
#else
	memcpy(buf, (void *)(BOARDCAL + offset), len);
#endif
}

/*function set_board_data()
 *description:
 *write data to the flash.
 * return value: 0 (success), 1 (fail)
 */
int set_board_data(int offset, int len, u8 *buf)
{
	char sectorBuff[CONFIG_SYS_FLASH_SECTOR_SIZE];
#ifdef DNI_NAND
	size_t read_size = CONFIG_SYS_FLASH_SECTOR_SIZE;
	nand_erase_options_t nand_erase_options;
	size_t write_size = CONFIG_SYS_FLASH_SECTOR_SIZE;
	int ret = 0;
#endif

#ifdef DNI_NAND
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	ret = nand_read_skip_bad_old(&nand_info[0], BOARDCAL, &read_size, (u_char *)sectorBuff);
#endif
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ret = nand_read_skip_bad(&nand_info[0], BOARDCAL, &read_size, NULL, CONFIG_SYS_FLASH_SECTOR_SIZE, (u_char *)sectorBuff);
#endif
	printf(" %zu bytes read: %s\n", read_size,
	       ret ? "ERROR" : "OK");
#else
	memcpy(sectorBuff, (void *)BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif
	memcpy(sectorBuff + offset, buf, len);
#ifdef DNI_NAND
	update_data(sectorBuff, CONFIG_SYS_FLASH_SECTOR_SIZE,
			BOARDCAL, BOARDCAL_LEN, 0, 0);
#else
	flash_erase(flash_info, CAL_SECTOR, CAL_SECTOR);
	write_buff(flash_info, sectorBuff, BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif
	return 0;
}

#ifdef CONFIG_TRIPLE_MAC_ADDRESS
int do_triple_macset(cmd_tbl_t *cmdtp, int flag, int argc,
                       char * const argv[])
{
#ifndef DNI_NAND
    char    sectorBuff[CONFIG_SYS_FLASH_SECTOR_SIZE];
#endif
    char    mac[6] = {255, 255, 255, 255, 255, 255}; // 255*6 = 1530
    int     mac_offset, i=0, j=0, val=0, sum=0;

    if(3 != argc)
        goto error;

    if(0 == strcmp(argv[1],"lan"))
        mac_offset = LAN_MAC_OFFSET;
    else if(0 == strcmp(argv[1],"wan"))
        mac_offset = WAN_MAC_OFFSET;
    else if(0 == strcmp(argv[1],"wlan5g"))
        mac_offset = WLAN_MAC_OFFSET;
    else
    {
        printf("unknown interface: %s\n",argv[1]);
        return 1;
    }

    while(argv[2][i])
    {
        if(':' == argv[2][i])
        {
            mac[j++] = val;
            i++;
            sum += val;
            val = 0;
            continue;
        }
        if((argv[2][i] >= '0') && (argv[2][i] <= '9'))
            val = val*16 + (argv[2][i] - '0');
        else if((argv[2][i] >='a') && (argv[2][i] <= 'f'))
            val = val*16 + (argv[2][i] - 'a') + 10;
        else if((argv[2][i] >= 'A') && (argv[2][i] <= 'F'))
            val = val*16 + (argv[2][i] - 'A') + 10;
        else
            goto error;
        i++;
    }
    mac[j] = val;
    sum += val;

    if(j != 5  || 0 == sum || 1530 == sum)
        goto error;

#ifdef DNI_NAND
    set_board_data(mac_offset, 6, mac);
#else
    memcpy(sectorBuff,(void *)BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);

    memcpy(sectorBuff + mac_offset, mac, 6);

        flash_sect_erase (BOARDCAL, BOARDCAL);
        flash_write (sectorBuff, BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif

    return 0;

error:
    printf("\nUBOOT-1.1.4 MACSET TOOL copyright.\n");
    printf("Usage:\n  macset lan(wan,wlan5g) address\n");
    printf("  For instance : macset lan 00:03:7F:EF:77:87\n");
    printf("  The MAC address can not be all 0x00 or all 0xFF\n");
    return 1;
}

U_BOOT_CMD(
    macset, 3, 0, do_triple_macset,
    "Set ethernet MAC address",
    "<interface> <address> - Program the MAC address of <interface>\n"
    "<interfcae> should be lan, wan or wlan5g\n"
    "<address> should be the format as 00:03:7F:EF:77:87"
);

int do_triple_macshow(cmd_tbl_t *cmdtp, int flag, int argc,
                        char * const argv[])
{
#ifdef DNI_NAND
    unsigned char mac[18];
#else
    unsigned char mac[CONFIG_SYS_FLASH_SECTOR_SIZE];
#endif

#ifdef DNI_NAND
    get_board_data(0, 18, mac);
#else
    memcpy(mac, (void *)BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif
    printf("lan mac: %02x:%02x:%02x:%02x:%02x:%02x\n",mac[LAN_MAC_OFFSET],mac[LAN_MAC_OFFSET+1],mac[LAN_MAC_OFFSET+2],mac[LAN_MAC_OFFSET+3],mac[LAN_MAC_OFFSET+4],mac[LAN_MAC_OFFSET+5]);
    printf("wan mac: %02x:%02x:%02x:%02x:%02x:%02x\n",mac[WAN_MAC_OFFSET],mac[WAN_MAC_OFFSET+1],mac[WAN_MAC_OFFSET+2],mac[WAN_MAC_OFFSET+3],mac[WAN_MAC_OFFSET+4],mac[WAN_MAC_OFFSET+5]);
    printf("wlan5g mac: %02x:%02x:%02x:%02x:%02x:%02x\n",mac[WLAN_MAC_OFFSET],mac[WLAN_MAC_OFFSET+1],mac[WLAN_MAC_OFFSET+2],mac[WLAN_MAC_OFFSET+3],mac[WLAN_MAC_OFFSET+4],mac[WLAN_MAC_OFFSET+5]);
    return 0;
}

U_BOOT_CMD(
    macshow, 1, 0, do_triple_macshow,
    "Show ethernet MAC addresses",
    "Display all the ethernet MAC addresses\n"
    "          for instance: the MAC of lan and wan"
);
#endif

#if defined(WPSPIN_OFFSET) && defined(WPSPIN_LENGTH)
int do_wpspinset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifndef DNI_NAND
	char sectorBuff[CONFIG_SYS_FLASH_SECTOR_SIZE];
#endif
	char wpspin[WPSPIN_LENGTH] = {0};

	if (2 != argc) {
		printf("%s\n", cmdtp->usage);
		return 1;
	}

	strncpy(wpspin, argv[1], WPSPIN_LENGTH);
#ifndef DNI_NAND
	memcpy(sectorBuff, (void *)BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
	memcpy(sectorBuff + WPSPIN_OFFSET, wpspin, WPSPIN_LENGTH);
#endif

	printf("Burn wpspin into ART block.\n");
#ifdef DNI_NAND
	set_board_data(WPSPIN_OFFSET, WPSPIN_LENGTH, wpspin);
#else
	flash_erase(flash_info, CAL_SECTOR, CAL_SECTOR);
	write_buff(flash_info, sectorBuff, BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif

	puts ("done\n");
	return 0;
}

U_BOOT_CMD(
	wpspinset, 2, 0, do_wpspinset,
	"Set wpspin number",
	"number\n"
	" For instance: wpspinset 12345678"
);

#endif

#if defined(SERIAL_NUMBER_OFFSET) && defined(SERIAL_NUMBER_LENGTH)
/*function do_snset()
 *description:
 *write the Serial Number to the flash.
 * return value:
 * 0:success
 * 1:fail
 */
int do_snset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifndef DNI_NAND
	char sectorBuff[CONFIG_SYS_FLASH_SECTOR_SIZE];
#endif
	char sn[SERIAL_NUMBER_LENGTH] = {0};
	int sn_len = 0, i = 0;

	if (2 != argc) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}

	sn_len = strlen(argv[1]);   /*check the SN's length*/
	if (sn_len != SERIAL_NUMBER_LENGTH) {
		printf ("SN's len is wrong,it's lenth is %d\n ", SERIAL_NUMBER_LENGTH);
		return 1;
	}

	strncpy(sn, argv[1], SERIAL_NUMBER_LENGTH);
	for (i=0; i<SERIAL_NUMBER_LENGTH; ++i)/*check seria naumber is 0~9 or A~Z*/
	{
		if (!(((sn[i]>=0x30) && (sn[i]<=0x39)) || ((sn[i]>=0x41) && (sn[i]<=0x5a))))    /*sn is 0~9 or A~Z*/
		{
			puts ("the SN only is 0~9 or A~Z\n");
			break;
		}
	}

	if (i < SERIAL_NUMBER_LENGTH)       /*because SN is not 0~9 or A~Z*/
		return 1;

#ifndef DNI_NAND
	memcpy(sectorBuff, (void *)BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
	memcpy(sectorBuff + SERIAL_NUMBER_OFFSET, sn, SERIAL_NUMBER_LENGTH);
#endif

	puts("Burn SN into ART block.\n");
#ifdef DNI_NAND
	set_board_data(SERIAL_NUMBER_OFFSET, SERIAL_NUMBER_LENGTH, sn);
#else
	flash_erase(flash_info, CAL_SECTOR, CAL_SECTOR);
	write_buff(flash_info, sectorBuff, BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif

	puts("Done.\n");
	return 0;
}

U_BOOT_CMD(
	snset, 2, 0, do_snset,
	"Set serial number",
	"number (13 digit)\n"
	" For instance: snset 1ML1747D0000B"
);
#endif

#if defined(REGION_NUMBER_OFFSET) && defined(REGION_NUMBER_LENGTH)
/*function set_region()
 *description:
 *write the Region Number to the flash.
 * return value:
 * 0:success
 * 1:fail
 */
int set_region(u16 host_region_number)
{
#ifndef DNI_NAND
	char sectorBuff[CONFIG_SYS_FLASH_SECTOR_SIZE];
#endif

	int rn_len = 0, i = 0;
	/* Always save region number as network order */
	u16 region_number = htons(host_region_number);

#ifndef DNI_NAND
	memcpy(sectorBuff, (void *)BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
	memcpy(sectorBuff + REGION_NUMBER_OFFSET, &region_number, REGION_NUMBER_LENGTH);
#endif

	puts("Burn Region Number into ART block.\n");
#ifdef DNI_NAND
	set_board_data(REGION_NUMBER_OFFSET, REGION_NUMBER_LENGTH, &region_number);
#else
	flash_erase(flash_info, CAL_SECTOR, CAL_SECTOR);
	write_buff(flash_info, sectorBuff, BOARDCAL, CONFIG_SYS_FLASH_SECTOR_SIZE);
#endif

	puts("Done.\n");
	return 0;
}

/*function do_rnset()
 *description:
 * read command input and translate to u16,
 * then call set_region() to write to flash.
 * return value:
 * 0:success
 * 1:fail
 */
int do_rnset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *strtol_endptr = NULL;
	uint16_t region = 0;

	int i = 0;

	if (2 != argc) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}

	region = (uint16_t)simple_strtoul(argv[1], &strtol_endptr, 10);
	if (*strtol_endptr != '\0') {
		printf("\"%s\" is not a number!!\n", argv[1]);
		return 1;
	}

	printf("write 0x%04x to board region\n", region);

	return set_region(region);

}

U_BOOT_CMD(
	rnset, 2, 0, do_rnset,
	"set region number",
	"number (2 digit)\n"
	" For instance: rnset 01"
);

int do_rnshow(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u16 rn;

#ifdef DNI_NAND
	get_board_data(REGION_NUMBER_OFFSET, sizeof(rn), &rn);
#else
	memcpy(&rn, (void *)(BOARDCAL + REGION_NUMBER_OFFSET), sizeof(rn));
#endif
	printf("region on board: 0x%04x\n", ntohs(rn));
	return 0;
}

U_BOOT_CMD(
	rnshow, 1, 0, do_rnshow,
	"Show Region Number on Board",
	"\n"
	" For instance: rnshow"
);
#endif

#if defined(BOARD_HW_ID_OFFSET) && defined(BOARD_HW_ID_LENGTH)
/*function do_board_hw_id_set()
 *description:
 * read in board_hw_id, then call set_board_data() to write to flash.
 * return value: 0 (success), 1 (fail)
 */
int do_board_hw_id_set(cmd_tbl_t *cmdtp, int flag, int argc,
                       char * const argv[])
{
	u8 board_hw_id[BOARD_HW_ID_LENGTH + 1];
	int board_hw_id_len = 0;

	if (argc != 2) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}
	if ((board_hw_id_len = strlen(argv[1])) > BOARD_HW_ID_LENGTH) {
		printf ("the length of BOARD_HW_ID can't > %d\n", BOARD_HW_ID_LENGTH);
		return 1;
	}

	memset(board_hw_id, 0, sizeof(board_hw_id));
	memcpy(board_hw_id, argv[1], board_hw_id_len);

	printf("Burn board_hw_id (= %s) into ART block\n", board_hw_id);
	set_board_data(BOARD_HW_ID_OFFSET, BOARD_HW_ID_LENGTH, board_hw_id);
	puts("Done.\n");
	return 0;
}

U_BOOT_CMD(
	board_hw_id_set, 2, 0, do_board_hw_id_set,
	"Set board_hw_id",
	"XXXXXX"
);

void get_board_hw_id(u8* buf) /* sizeof(buf) must > BOARD_HW_ID_LENGTH */
{
	get_board_data(BOARD_HW_ID_OFFSET, BOARD_HW_ID_LENGTH, buf);
}

int do_board_hw_id_show(cmd_tbl_t *cmdtp, int flag, int argc,
                        char * const argv[])
{
	u8 board_hw_id[BOARD_HW_ID_LENGTH + 1];

	memset(board_hw_id, 0, sizeof(board_hw_id));
	get_board_hw_id(board_hw_id);
	printf("board_hw_id : %s\n", board_hw_id);
	return 0;
}

U_BOOT_CMD(
	board_hw_id_show, 1, 0, do_board_hw_id_show,
	"Show board_hw_id",
	"\n"
	" For instance: board_hw_id_show"
);

#if defined(BOARD_MODEL_ID_OFFSET) && defined(BOARD_MODEL_ID_LENGTH)
/*function do_board_model_id_set()
 *description:
 * read in board_model_id, then call set_board_data() to write to flash.
 * return value: 0 (success), 1 (fail)
 */
int do_board_model_id_set(cmd_tbl_t *cmdtp, int flag, int argc,
                          char * const argv[])
{
	u8 board_model_id[BOARD_MODEL_ID_LENGTH + 1];
	int board_model_id_len = 0;

	if (argc != 2) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}
	if ((board_model_id_len = strlen(argv[1])) > BOARD_MODEL_ID_LENGTH) {
		printf ("the length of BOARD_MODEL_ID can't > %d\n", BOARD_MODEL_ID_LENGTH);
		return 1;
	}

	memset(board_model_id, 0, sizeof(board_model_id));
	memcpy(board_model_id, argv[1], board_model_id_len);

	printf("Burn board_model_id (= %s) into ART block\n", board_model_id);
	set_board_data(BOARD_MODEL_ID_OFFSET, BOARD_MODEL_ID_LENGTH, board_model_id);
	puts("Done.\n");
	return 0;
}

U_BOOT_CMD(
	board_model_id_set, 2, 0, do_board_model_id_set,
	"Set board_model_id",
	"XXXXXX"
);

void get_board_model_id(u8* buf) /* sizeof(buf) must > BOARD_MODEL_ID_LENGTH */
{
	get_board_data(BOARD_MODEL_ID_OFFSET, BOARD_MODEL_ID_LENGTH, buf);
}

int do_board_model_id_show(cmd_tbl_t *cmdtp, int flag, int argc,
                           char * const argv[])
{
	u8 board_model_id[BOARD_MODEL_ID_LENGTH + 1];

	memset(board_model_id, 0, sizeof(board_model_id));
	get_board_model_id(board_model_id);
	printf("board_model_id : %s\n", board_model_id);
	return 0;
}

U_BOOT_CMD(
	board_model_id_show, 1, 0, do_board_model_id_show,
	"Show board_model_id",
	"\n"
	" For instance: board_model_id_show"
);
#endif	/* MODEL_ID */
#endif	/* HW_ID */

#if defined(BOARD_SSID_OFFSET) && defined(BOARD_SSID_LENGTH)
/*function do_board_ssid_set()
 *description:
 * read in ssid, then call set_board_data() to write to flash.
 * return value: 0 (success), 1 (fail)
 */
int do_board_ssid_set(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	u8 board_ssid[BOARD_SSID_LENGTH + 1];
	int board_ssid_len = 0;

	if (argc != 2) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}
	if ((board_ssid_len = strlen(argv[1])) > BOARD_SSID_LENGTH) {
		printf ("the length of SSID can't > %d\n", BOARD_SSID_LENGTH);
		return 1;
	}

	memset(board_ssid, 0, sizeof(board_ssid));
	memcpy(board_ssid, argv[1], board_ssid_len);

	printf("Burn SSID (= %s) into ART block\n", board_ssid);
	set_board_data(BOARD_SSID_OFFSET, BOARD_SSID_LENGTH, board_ssid);
	puts("Done.\n");
	return 0;
}

U_BOOT_CMD(
	board_ssid_set, 2, 0, do_board_ssid_set,
	"Set ssid on board",
	"XXXXXX\n"
	" For instance: board_ssid_set NETGEAR"
);

void get_board_ssid(u8* buf) /* sizeof(buf) must > BOARD_SSID_LENGTH */
{
	get_board_data(BOARD_SSID_OFFSET, BOARD_SSID_LENGTH, buf);
}

int do_board_ssid_show(cmd_tbl_t *cmdtp, int flag, int argc,
                       char * const argv[])
{
	u8 board_ssid[BOARD_SSID_LENGTH + 1];

	memset(board_ssid, 0, sizeof(board_ssid));
	get_board_ssid(board_ssid);
	printf("board_ssid : %s\n", board_ssid);
	return 0;
}

U_BOOT_CMD(
	board_ssid_show, 1, 0, do_board_ssid_show,
	"Show board_ssid",
	"\n"
	" For instance: board_ssid_show"
);

#endif	/* BOARD_SSID */

#if defined(BOARD_PASSPHRASE_OFFSET) && defined(BOARD_PASSPHRASE_LENGTH)
/*function do_board_passphrase_set()
 *description:
 * read in passphrase, then call set_board_data() to write to flash.
 * return value: 0 (success), 1 (fail)
 */
int do_board_passphrase_set(cmd_tbl_t *cmdtp, int flag, int argc,
                            char * const argv[])
{
	u8 board_passphrase[BOARD_PASSPHRASE_LENGTH + 1];
	int board_passphrase_len = 0;

	if (argc != 2) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}
	if ((board_passphrase_len = strlen(argv[1])) > BOARD_PASSPHRASE_LENGTH) {
		printf ("the length of PASSPHRASE can't > %d\n", BOARD_PASSPHRASE_LENGTH);
		return 1;
	}

	memset(board_passphrase, 0, sizeof(board_passphrase));
	memcpy(board_passphrase, argv[1], board_passphrase_len);

	printf("Burn PASSPHRASE (= %s) into ART block\n", board_passphrase);
	set_board_data(BOARD_PASSPHRASE_OFFSET, BOARD_PASSPHRASE_LENGTH, board_passphrase);
	puts("Done.\n");
	return 0;
}

U_BOOT_CMD(
	board_passphrase_set, 2, 0, do_board_passphrase_set,
	"Set passphrase on board",
	"XXXXXX\n"
	" For instance: board_passphrase_set 1234567890"
);

void get_board_passphrase(u8* buf) /* sizeof(buf) must > BOARD_PASSPHRASE_LENGTH */
{
	get_board_data(BOARD_PASSPHRASE_OFFSET, BOARD_PASSPHRASE_LENGTH, buf);
}

int do_board_passphrase_show(cmd_tbl_t *cmdtp, int flag, int argc,
                             char * const argv[])
{
	u8 board_passphrase[BOARD_PASSPHRASE_LENGTH + 1];

	memset(board_passphrase, 0, sizeof(board_passphrase));
	get_board_passphrase(board_passphrase);
	printf("board_passphrase : %s\n", board_passphrase);
	return 0;
}

U_BOOT_CMD(
	board_passphrase_show, 1, 0, do_board_passphrase_show,
	"Show board_passphrase",
	"\n"
	" For instance: board_passphrase_show"
);
#endif	/* BOARD_PASSPHRASE */

#if defined(CONFIG_CMD_BOARD_PARAMETERS) \
&& defined(LAN_MAC_OFFSET) && defined(WAN_MAC_OFFSET) \
&& defined(WPSPIN_LENGTH) && defined(WPSPIN_OFFSET) && defined(SERIAL_NUMBER_LENGTH) && defined(SERIAL_NUMBER_OFFSET) \
&& defined(REGION_NUMBER_LENGTH) && defined(REGION_NUMBER_OFFSET) && defined(BOARD_HW_ID_LENGTH) && defined(BOARD_HW_ID_OFFSET) \
&& defined(BOARD_MODEL_ID_LENGTH) && defined(BOARD_MODEL_ID_OFFSET) && defined(BOARD_SSID_LENGTH) && defined(BOARD_SSID_OFFSET) \
&& defined(BOARD_PASSPHRASE_LENGTH) && defined(BOARD_PASSPHRASE_OFFSET)
int do_board_parameters_set(cmd_tbl_t *cmdtp, int flag, int argc,
                            char * const argv[])
{
	char sectorBuff[6 * 3 +
         WPSPIN_LENGTH + SERIAL_NUMBER_LENGTH +
         REGION_NUMBER_LENGTH + BOARD_HW_ID_LENGTH +
         BOARD_MODEL_ID_LENGTH + BOARD_SSID_LENGTH +
         BOARD_PASSPHRASE_LENGTH 
#if defined(BLUETOOTH_MAC_OFFSET)
	+ BLUETOOTH_MAC_LENGTH
#endif

#if defined(SFP_MAC_OFFSET)
         +SFP_MAC_LENGTH 
#endif

#if defined(WLAN11AD_MAC_OFFSET)
	+WLAN11AD_MAC_LENGTH
#endif
	];

	u8 mac[6][6] = {
	    {255, 255, 255, 255, 255, 255},
                    {255, 255, 255, 255, 255, 255},
                    {255, 255, 255, 255, 255, 255},
                    {255, 255, 255, 255, 255, 255},
                    {255, 255, 255, 255, 255, 255},
                    {255, 255, 255, 255, 255, 255}
                 };


	char wpspin[WPSPIN_LENGTH] = {0};
	char sn[SERIAL_NUMBER_LENGTH] = {0};
	u8 board_ssid[BOARD_SSID_LENGTH + 1];
	u8 board_passphrase[BOARD_PASSPHRASE_LENGTH + 1];
	int wps_len = 0;
	int sn_len = 0;
	int board_passphrase_len = 0;
	int board_ssid_len = 0;
	int offset, i=0, j=0, k=0, val=0, sum=0, length, mac_num=2, m=0;

	if (argc > 11 || argc < 7) {
		printf("%s\n",cmdtp->usage);
		return 1;
	}
	if (argc >= 8)
		mac_num += argc -7 ;
	printf("mac_num:%d\n",mac_num);	

	length = 6 * mac_num + WPSPIN_LENGTH + SERIAL_NUMBER_LENGTH +
					REGION_NUMBER_LENGTH + BOARD_HW_ID_LENGTH +
					BOARD_MODEL_ID_LENGTH + BOARD_SSID_LENGTH +
					BOARD_PASSPHRASE_LENGTH ;

	/* Check WPS length */
	if ((wps_len = strlen(argv[1])) > WPSPIN_LENGTH)
	{
		printf ("the length of wpspin can't > %d\n", WPSPIN_LENGTH);
		return 1;
	}

	/* Check serial number */
	sn_len = strlen(argv[2]);   /*check the SN's length*/
	if (sn_len != SERIAL_NUMBER_LENGTH)
	{
		printf ("SN's len is wrong,it's lenth is %d\n ", SERIAL_NUMBER_LENGTH);
		return 1;
	}

	strncpy(sn, argv[2], SERIAL_NUMBER_LENGTH);
	for (i = 0; i < SERIAL_NUMBER_LENGTH; ++i)/*check seria naumber is 0~9 or A~Z*/
	{
		if (!(((sn[i]>=0x30) && (sn[i]<=0x39)) || ((sn[i]>=0x41) && (sn[i]<=0x5a))))    /*sn is 0~9 or A~Z*/
		{
			puts ("the SN only is 0~9 or A~Z\n");
			break;
		}
	}

	if (i < SERIAL_NUMBER_LENGTH)       /*because SN is not 0~9 or A~Z*/
		return 1;

	/* Check SSID length */
	if ((board_ssid_len = strlen(argv[3])) > BOARD_SSID_LENGTH)
	{
		printf ("the length of SSID can't > %d\n", BOARD_SSID_LENGTH);
		return 1;
	}

	/* Check Passphrase length */
	if ((board_passphrase_len = strlen(argv[4])) > BOARD_PASSPHRASE_LENGTH)
	{
		printf ("the length of PASSPHRASE can't > %d\n", BOARD_PASSPHRASE_LENGTH);
		return 1;
	}

	/* check MAC address */
	for (k = 5; k < 5 + mac_num; ++k)
	{
		sum = 0 , val = 0;
		i = 0; j = 0;
		while (argv[k][i])
		{
			if (':' == argv[k][i])
			{
				mac[m][j++] = val;
				i++;
				sum += val;
				val = 0;
				continue;
			}
			if ((argv[k][i] >= '0') && (argv[k][i] <= '9'))
				val = val*16 + (argv[k][i] - '0');
			else if ((argv[k][i] >='a') && (argv[k][i] <= 'f'))
				val = val*16 + (argv[k][i] - 'a') + 10;
			else if ((argv[k][i] >= 'A') && (argv[k][i] <= 'F'))
				val = val*16 + (argv[k][i] - 'A') + 10;
			else
			{
				printf("The %d MAC address is incorrect\n",k);
				printf("The MAC address can not be all 0x00 or all 0xFF\n");
				return 1;
			}
			i++;
		}
		mac[m][j] = val;
		sum += val;
		m++;
		if (j != 5  || 0 == sum || 1530 == sum)
		{
			printf("The %d MAC address is incorrect\n",k);
			printf("The MAC address can not be all 0x00 or all 0xFF\n");
			return 1;
		}
	}


	/* Copy new settings to buffer */
	get_board_data(0, length, sectorBuff);
	if(mac_num <= 3)
		memcpy(sectorBuff, mac, 6 * mac_num);
	else{
	        memcpy(sectorBuff, mac, 6 * 3);
		memcpy(sectorBuff + BLUETOOTH_MAC_OFFSET, mac[3], 6 * (mac_num-3) );
	}
	memset(wpspin, 0, sizeof(wpspin));
	memcpy(wpspin, argv[1], wps_len);
	memcpy(sectorBuff + WPSPIN_OFFSET, wpspin, WPSPIN_LENGTH);
	memcpy(sectorBuff + SERIAL_NUMBER_OFFSET, sn, SERIAL_NUMBER_LENGTH);
	memset(board_ssid, 0, sizeof(board_ssid));
	memcpy(board_ssid, argv[3], board_ssid_len);
	memcpy(sectorBuff + BOARD_SSID_OFFSET, board_ssid, BOARD_SSID_LENGTH);
	memset(board_passphrase, 0, sizeof(board_passphrase));
	memcpy(board_passphrase, argv[4], board_passphrase_len);
	memcpy(sectorBuff + BOARD_PASSPHRASE_OFFSET, board_passphrase, BOARD_PASSPHRASE_LENGTH);

	printf("Burn the following parameters into ART block.\n");
	printf("lan mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[0][0],mac[0][1],mac[0][2],mac[0][3],mac[0][4],mac[0][5]);
	printf("wan mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[1][0],mac[1][1],mac[1][2],mac[1][3],mac[1][4],mac[1][5]);
	if (mac_num >= 3)
		printf("wlan5g mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[2][0],mac[2][1],mac[2][2],mac[2][3],mac[2][4],mac[2][5]);
	for(i=3;i<=mac_num;i++){
		if(i==4) printf("BLUETOOTH mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[3][0],mac[3][1],mac[3][2],mac[3][3],mac[3][4],mac[3][5]);
		if(i==5) printf("SFP+ mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[4][0],mac[4][1],mac[4][2],mac[4][3],mac[4][4],mac[4][5]);
		if(i==6) printf("WLAN11AD mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[5][0],mac[5][1],mac[5][2],mac[5][3],mac[5][4],mac[5][5]);
	}

	printf("WPSPIN code: ");
	if(mac_num >3) mac_num=3 ; 
	offset = 6*mac_num + WPSPIN_LENGTH;
	for (i = 6*mac_num; i < offset; ++i)
		printf("%c",sectorBuff[i]);
	printf("\nSerial Number: ");
	offset += SERIAL_NUMBER_LENGTH;
	for (; i < offset; ++i)
		printf("%c",sectorBuff[i]);
	printf("\nSSID: ");
	offset = offset + REGION_NUMBER_LENGTH + BOARD_HW_ID_LENGTH + BOARD_MODEL_ID_LENGTH + BOARD_SSID_LENGTH;
	i = i + REGION_NUMBER_LENGTH + BOARD_HW_ID_LENGTH + BOARD_MODEL_ID_LENGTH;
	for (; i < offset; ++i)
		printf("%c",sectorBuff[i]);
	printf("\nPASSPHRASE: ");
	offset += BOARD_PASSPHRASE_LENGTH;
	for (; i < offset; ++i)
		printf("%c",sectorBuff[i]);
	printf("\n\n");

	set_board_data(0, length, sectorBuff);

	return 0;
}

U_BOOT_CMD(
	board_parameters_set, 11, 0, do_board_parameters_set,
	"Set WPS PIN code, Serial number, SSID, Passphrase, MAC address",
	"<WPS Pin> <SN> <SSID> <PASSPHRASE> <lan address> <wan address> [optional: <wlan5g address>]\n"
	"[optional: bluetooth address] [optional: SFP+ address] [optional: 11AD address]\n"
	"          <WPS Pin> (8 digits)\n"
	"          <SN> Serial number (13 digits)\n"
	"          <SSID> SSID (max 32 digits)\n"
	"          <PASSPHRASE> Passphrase (max 64 digits)\n"
	"          <[lan|wan|wlan5g] address> should be the format as 00:03:7F:EF:77:87\n"
	" For instance: board_parameters_set 12345678 1ML1747D0000B GAEGTEN 1234567890 00:03:7F:EF:77:87 00:03:33:44:66:FE 00:03:77:66:77:88"
);

int do_board_parameters_show(cmd_tbl_t * cmdtp, int flag, int argc,
                             char * const argv[])
{
	unsigned char sectorBuff[CONFIG_SYS_FLASH_SECTOR_SIZE];
	int i, end;

	get_board_data(0, CONFIG_SYS_FLASH_SECTOR_SIZE, sectorBuff);

	printf("WPSPIN code: ");
	end = WPSPIN_OFFSET + WPSPIN_LENGTH;
	for (i = WPSPIN_OFFSET; i < end; ++i)
		printf("%c",sectorBuff[i]);

	printf("\nSerial Number: ");
	end = SERIAL_NUMBER_OFFSET + SERIAL_NUMBER_LENGTH;
	for (i = SERIAL_NUMBER_OFFSET; i < end; ++i)
		printf("%c",sectorBuff[i]);

	printf("\nSSID: ");
	end = BOARD_SSID_OFFSET + BOARD_SSID_LENGTH;
	for (i = BOARD_SSID_OFFSET; i < end; ++i)
		printf("%c",sectorBuff[i]);

	printf("\nPASSPHRASE: ");
	end = BOARD_PASSPHRASE_OFFSET + BOARD_PASSPHRASE_LENGTH;
	for (i = BOARD_PASSPHRASE_OFFSET; i < end; ++i)
		printf("%c",sectorBuff[i]);
	printf("\n");

	i = LAN_MAC_OFFSET;
	printf("lan mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			sectorBuff[i], sectorBuff[i+1], sectorBuff[i+2],
			sectorBuff[i+3], sectorBuff[i+4], sectorBuff[i+5]);

	i = WAN_MAC_OFFSET;
	printf("wan mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			sectorBuff[i], sectorBuff[i+1], sectorBuff[i+2],
			sectorBuff[i+3], sectorBuff[i+4], sectorBuff[i+5]);

#if defined WLAN_MAC_OFFSET
	i = WLAN_MAC_OFFSET;
	printf("wlan5g mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			sectorBuff[i], sectorBuff[i+1], sectorBuff[i+2],
			sectorBuff[i+3], sectorBuff[i+4], sectorBuff[i+5]);
#endif

#if defined BLUETOOTH_MAC_OFFSET
	i = BLUETOOTH_MAC_OFFSET;
	printf("BLUETOOTH mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			sectorBuff[i], sectorBuff[i+1], sectorBuff[i+2],
			sectorBuff[i+3], sectorBuff[i+4], sectorBuff[i+5]);
#endif

#if defined SFP_MAC_OFFSET
	i = SFP_MAC_OFFSET;
	printf("SFP+ mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			sectorBuff[i], sectorBuff[i+1], sectorBuff[i+2],
			sectorBuff[i+3], sectorBuff[i+4], sectorBuff[i+5]);
#endif

#if defined WLAN11AD_MAC_OFFSET
	i = WLAN11AD_MAC_OFFSET;
	printf("WLAN11AD mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
			sectorBuff[i], sectorBuff[i+1], sectorBuff[i+2],
			sectorBuff[i+3], sectorBuff[i+4], sectorBuff[i+5]);
#endif
	return 0;
}

U_BOOT_CMD(
	board_parameters_show, 1, 0, do_board_parameters_show,
	"Show WPS PIN code, Serial number, SSID, Passphrase, MAC address.",
	"\n"
	"- show <WPS Pin> <SN> <SSID> <PASSPHRASE> <lan address> <wan address>\n"
	"   [optional: <wlan5g address>] [optional: bluetooth address] [optional: SFP+ address] [optional: 11AD address]\n"
);
#endif  /* CONFIG_CMD_BOARD_PARAMETERS */

#endif /* BOARDCAL */

#ifdef CHECK_DNI_FIRMWARE_INTEGRITY
#ifdef DNI_NAND
/**
 * dni_nand_read_eb_manage_bad:
 *
 * Read a erase block (eb) from NAND flash.
 *
 * Blocks that have been marked bad are skipped and the next block is read
 * instead as long as the image is short enough to fit even after skipping the
 * bad blocks.
 *
 * BECAREFUL! If a block is found unable to be corrected by data in OOB, the
 * block is marked as bad block if marking bad block function is implemented
 * in NAND flash driver. **If you do not want to try experimental
 * marking-bad-block function, use nand_read_skip_bad() in
 * drivers/mtd/nand/nand_util.c instead.**
 *
 * @param nand       NAND device
 * @param block_num  Block number to be read. Block number of the first block
 *                   on NAND flash is 0.
 *                   When leaving this function, it equals to block number
 *                   which is truly read after bad blocks are skipped.
 * @param buffer     buffer to write to
 * @return 0 in case of success
 */
int dni_nand_read_eb_manage_bad(nand_info_t *nand, ulong *block_num,
                                u_char *buffer)
{
	int rval;
	uint64_t block_offset = (*block_num) * CONFIG_SYS_FLASH_SECTOR_SIZE;
	size_t read_length = CONFIG_SYS_FLASH_SECTOR_SIZE;

	while (block_offset < nand->size &&
	       nand_block_isbad(nand, block_offset)) {
		printf("Skipping bad block 0x%08llx\n", block_offset);

		block_offset += CONFIG_SYS_FLASH_SECTOR_SIZE;
		(*block_num)++;
	}

	if (block_offset >= nand->size) {
		printf("Attempt to read outside the flash area\n");
		return -EINVAL;
	}
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	char runcmd[256];
	snprintf(runcmd, sizeof(runcmd), "nand set_partition_offset 0x%lx", block_offset);
	printf("runcommand %s\n" , runcmd);
	if (run_command(runcmd, 0) != CMD_RET_SUCCESS) {
		printf ("nand set_partition_offset error\n");
	}
	rval = nand_read_skip_bad(nand, block_offset, &read_length, NULL, 0x20000, (u_char *)buffer);
	/* Set partition_offset back to $nand_pt_addr_al_boot after */
	/* calling nand_read_skip_bad function                      */
	snprintf(runcmd, sizeof(runcmd), "nand set_partition_offset 0x0");
	printf("runcommand %s\n" , runcmd);
	if (run_command(runcmd, 0) != CMD_RET_SUCCESS) {
		printf ("nand set_partition_offset error\n");
	}
#endif
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	rval = nand_read(nand, block_offset, &read_length, buffer);
#endif

	if (rval == -EBADMSG && mtd_block_markbad != NULL) {
		printf("Block 0x%llx is marked as bad block!!\n",
		       block_offset);
		mtd_block_markbad(nand, block_offset);
		do_reset(NULL, 0, 0, NULL);

	} else if (rval && rval != -EUCLEAN) {
		printf("NAND read from block %llx failed %d\n", block_offset,
		       rval);
		return rval;
	}
	return 0;
}

int do_loadn_dniimg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int dev;
	ulong offset, addr, kernel_partition_size;
	ulong addr_end;
	ulong block_num;
	image_header_t *hdr;
	nand_info_t *nand;

	if (argc != 4) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	dev = simple_strtoul(argv[1], NULL, 16);
	offset = simple_strtoul(argv[2], NULL, 16);
	addr = simple_strtoul(argv[3], NULL, 16);
	if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE || !nand_info[dev].name) {
		printf("\n** Device %d not available\n", dev);
		return 1;
	}

	nand = &nand_info[dev];
	printf("\nLoading from device %d: %s (offset 0x%lx)\n",
	       dev, nand->name, offset);

	block_num = offset / CONFIG_SYS_FLASH_SECTOR_SIZE;
	if (dni_nand_read_eb_manage_bad(
			nand, &block_num, (u_char *)addr)) {
		printf("** Read error on %d\n", dev);
		return 1;
	}
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	addr=addr+0x4;
#endif
	hdr = (image_header_t *)addr;
	kernel_partition_size = (((2 * sizeof(image_header_t) + ntohl(hdr->ih_size))
	           / CONFIG_SYS_FLASH_SECTOR_SIZE) + 1) * CONFIG_SYS_FLASH_SECTOR_SIZE;
	
	printf("\n** KERNEL partition size, kernel : 0x%x **\n", kernel_partition_size);
	
	if (!board_model_id_match_open_source_id() &&
	    kernel_partition_size > CONFIG_SYS_IMAGE_LEN) {
		printf("\n** Bad partition size, kernel : 0x%x **\n", kernel_partition_size);
		return 1;
	}

	addr_end = addr + kernel_partition_size;

	/* The first block is read. Start reading from the second block. */
	block_num++;
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	addr = addr - 0x4;
#endif
	addr += CONFIG_SYS_FLASH_SECTOR_SIZE;

	while (addr < addr_end) {
		if (dni_nand_read_eb_manage_bad(
				nand, &block_num, (u_char *)addr)) {
			printf("** Read kernel partition error on %d\n", dev);
			return 1;
		}
		block_num++;
		addr += CONFIG_SYS_FLASH_SECTOR_SIZE;
	}

#ifdef CHECK_DNI_FIRMWARE_ROOTFS_INTEGRITY
	ulong rsize;

	if (board_model_id_match_open_source_id())
		return 0;

#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	addr_end = addr_end - 0x4;
#endif
	hdr = (image_header_t *)(addr_end - sizeof(image_header_t));
	rsize = ntohl(hdr->ih_size);
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4)
	if( gpio_get_value(POWER_LED) ) {
		gpio_set_value(POWER_LED,0);
	} else {
		gpio_set_value(POWER_LED,1);
	}
#endif
	
	printf("\n** ROOTFS partition size, kernel: 0x%x, rootfs: 0x%x **\n", kernel_partition_size, rsize);
	if (rsize > (CONFIG_SYS_IMAGE_LEN - kernel_partition_size)) {
		printf("\n** Bad partition size, kernel: 0x%x, rootfs: 0x%x **\n", kernel_partition_size, rsize);
		return 1;
	}

	addr_end += rsize;
	while (addr < addr_end) {
		if (dni_nand_read_eb_manage_bad(
				nand, &block_num, (u_char *)addr)) {
			printf("** Read rootfs partition error on %d\n", dev);
			return 1;
		}
		block_num++;
		addr += CONFIG_SYS_FLASH_SECTOR_SIZE;
	}
#endif

	return 0;
}

U_BOOT_CMD(
	loadn_dniimg,	4,	0,	do_loadn_dniimg,
	"load dni firmware image from NAND.",
	"<device> <offset> <loadaddr>\n"
	"    - load dni firmware image that stored in NAND.\n"
	"    <device> : which NAND device.\n"
	"    <offset> : offset of the image in NAND.\n"
	"    <loadaddr> : address the image will be loaded to.\n"
);
#endif

int do_calculate_rootfs_uimage_address(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong offset, addr, mem_addr, rootfs_addr;
	ulong block_num;
	image_header_t *hdr;
	nand_info_t *nand;
	int dev = simple_strtoul(0, NULL, 16);
	nand = &nand_info[dev];	

	offset = simple_strtoul(argv[1], NULL, 16);
	addr = simple_strtoul(argv[2], NULL, 16);
	mem_addr = simple_strtoul(argv[2], NULL, 16);

	block_num = offset / CONFIG_SYS_FLASH_SECTOR_SIZE;
	dni_nand_read_eb_manage_bad(nand, &block_num, (u_char *)addr);

	addr=addr+0x4;

	hdr = (image_header_t *)addr;
	
	rootfs_addr = (ntohl(hdr->ih_size)/CONFIG_SYS_FLASH_SECTOR_SIZE+1)*CONFIG_SYS_FLASH_SECTOR_SIZE+2*sizeof(image_header_t)-sizeof(image_header_t);
	rootfs_addr = rootfs_addr - (0x80 - mem_addr);
	
	printf("\n** rootfs address : 0x%x **\n", rootfs_addr);
	char runcmd[256];
	snprintf(runcmd, sizeof(runcmd), "setenv rootfs_addr_for_fw_checking 0x%x",rootfs_addr);
	run_command(runcmd,0);

	return rootfs_addr;
}

U_BOOT_CMD(
	calc_rootaddr,   3,  0, do_calculate_rootfs_uimage_address,
	"Calculate the DRAM address of rootfs uImage.",
	" <offset>\n"
	"    - Calculate the DRAM address of rootfs uImage w.r.t. <loadaddr>.\n"
	"    <offset> : offset of the image in NAND.\n"
	"    <loadaddr> : address the image will be loaded to.\n"
);



int chk_img (ulong addr)
{
	ulong data, len, checksum;
	image_header_t header;
	image_header_t *hdr = &header;

	memmove (&header, (char *)addr, sizeof(image_header_t));
	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
		printf("\n** Bad Magic Number 0x%x **\n", hdr->ih_magic);
		return 1;
	}

	data = (ulong)&header;
	len  = sizeof(image_header_t);
	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;
	if (crc32 (0, (uchar *)data, len) != checksum) {
		puts ("\n** Bad Header Checksum **\n");
		return 1;
	}

	data = addr + sizeof(image_header_t);
	len  = ntohl(hdr->ih_size);
	puts ("   Verifying Checksum ... ");
	if (crc32 (0, (uchar *)data, len) != ntohl(hdr->ih_dcrc)) {
		puts ("   Bad Data CRC\n");
		return 1;
	}
	puts ("OK\n");

	return 0;
}

int do_chk_dniimg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong addr;

	if (board_model_id_match_open_source_id())
		return 0;

	if (argc != 2) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	addr = simple_strtoul(argv[1], NULL, 16);

	printf("\n** check kernel image **\n");
	if (chk_img(addr)) {
		return 1;
	}

#ifdef CHECK_DNI_FIRMWARE_ROOTFS_INTEGRITY
	image_header_t *hdr;
	ulong kernel_partition_size;

	hdr = (image_header_t *)addr;
	kernel_partition_size = (((2 * sizeof(image_header_t) + ntohl(hdr->ih_size))
	           / CONFIG_SYS_FLASH_SECTOR_SIZE) + 1) * CONFIG_SYS_FLASH_SECTOR_SIZE;

	printf("\n** check rootfs image **\n");
	if (chk_img(addr + kernel_partition_size - sizeof(image_header_t))) {
		return 1;
	}
#endif

	return 0;
}

U_BOOT_CMD(
	chk_dniimg,	2,	0,	do_chk_dniimg,
	"check integrity of dni firmware image.",
	"<addr> - check integrity of dni firmware image.\n"
	"    <addr> : starting address of image.\n"
);
#endif	/* CHECK_DNI_FIRMWARE_INTEGRITY */
