#ifndef __AL_ALPINE_HW29765235P0P512P1024P4X4P4X4_32_DB_H
#define __AL_ALPINE_HW29765235P0P512P1024P4X4P4X4_32_DB_H

#define TIGER_WIFI_BOARD
#define CONFIG_QCA8337_SWITCH
#define NO_BOARD_INFO_GPIO
#define GPIO_ETH_PHY_A_RESET 45
#define GPIO_ETH_PHY_B_RESET 46
#define GPIO_ETH_PHY_RESET_VALUE 1
#define NO_PCI_CLEANUP
#define CONFIG_PREBOOT
#define NAND_PT_SIZE "0x1f000000\0"

#include "alpine_db_common.h"
#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND "nmrp;" \
		"run check_dni_image;"	\
		"run bootargsnand;"	\
		"qca8337_init_one; ledtoggle;sleep 1;run bootnand"

#undef CONFIG_ENV_IS_IN_SPI_FLASH

#undef CONFIG_PCI_CLEANUP
#undef CONFIG_AL_NAND_ENV_COMMANDS
#undef CONFIG_EXTRA_ENV_SETTINGS

#ifndef PREBOOT_ENV_SETTINGS
#define PREBOOT_ENV_SETTINGS  ""
#endif

#ifndef NAND_PT_SIZE
#define NAND_PT_SIZE	"0x3f000000\0"
#endif

#define CONFIG_CMD_DNI
#define NETGEAR_BOARD_ID_SUPPORT

#define CONFIG_AL_NAND_ENV_COMMANDS					\
	"nand_pt_addr_al_boot=0x0\0"					\
	"nand_pt_addr_kernel=0x00580000\0"				\
	"nand_pt_size_kernel=0x00c00000\0"				\
	"nand_pt_addr_fs=0x01000000\0"					\
	"nand_pt_size_fs="NAND_PT_SIZE					\
	"nand_pt_desc_kernel_1=Test kernel A\0"				\
	"nand_pt_addr_kernel_1=0x00400000\0"				\
	"nand_pt_desc_kernel_2=Test kernel B\0"				\
	"nand_pt_addr_kernel_2=0x00a00000\0"

#define CONFIG_EXTRA_ENV_SETTINGS					\
	PREBOOT_ENV_SETTINGS						\
	"ethprime=al_eth1\0"						\
	"nfsrootdir="							\
		"/srv/root/\0"						\
	"autoload="							\
		"n\0"							\
	"fail="								\
		"echo Failed!; lcd_print \"Failed!\"\0"			\
	"kernelupd="							\
		"lcd_print \"Updating kernel...\";"			\
		"tftpboot $loadaddr_payload ${tftpdir}uImage;"		\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"nand set_partition_offset $nand_pt_addr_kernel;"	\
		"mw.l $loadaddr $filesize;"				\
		"incenv filesize 4;"					\
		"nand erase.spread $nand_pt_addr_kernel $filesize;"	\
		"nand write $loadaddr $nand_pt_addr_kernel $filesize;"	\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"echo kernelupd done;"					\
		"lcd_print \"Done\"\0"					\
	"rootfsupd="							\
		"lcd_print \"Updating rootfs...\";"			\
		"tftpboot $loadaddr_rootfs_chk ${tftpdir}rootfs.ubi.md5;"	\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"tftpboot $loadaddr ${tftpdir}rootfs.ubi;"		\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"md5sum -v $loadaddr $filesize *$loadaddr_rootfs_chk;"	\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"nand set_partition_offset $nand_pt_addr_fs;"		\
		"nand erase.spread $nand_pt_addr_fs $nand_pt_size_fs;"	\
		"nand write $loadaddr $nand_pt_addr_fs $filesize;"	\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"echo rootfsupd done;"					\
		"lcd_print \"Done\"\0"					\
	"bootnand="							\
		"lcd_print \"Loading OS...\";"				\
		"nand set_partition_offset $nand_pt_addr_kernel;"		\
		"nand read $loadaddr $nand_pt_addr_kernel 4;"		\
		"setenvmem filesize $loadaddr;"				\
		"incenv filesize 4;"					\
		"nand read $loadaddr $nand_pt_addr_kernel $filesize;"	\
		"ledtoggle;"						\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"bootm $loadaddr_payload - $fdtaddr;"			\
		"lcd_print Failed!\0"					\
	"check_dni_image="							\
		"lcd_print \"Loading DNI firmware for checking...\";"				\
		"nand set_partition_offset $nand_pt_addr_kernel;"		\
		"ledtoggle;"                                                    \
		"loadn_dniimg 0 $nand_pt_addr_kernel $loadaddr;"		\
	        "ledtoggle;"                                                   \
		"calc_rootaddr $nand_pt_addr_kernel $loadaddr;"			\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"setenv kernel_addr_for_fw_checking $loadaddr;"			\
		"incenv kernel_addr_for_fw_checking 4;"					\
		"iminfo $kernel_addr_for_fw_checking;"					\
		"if test $? -ne 0; then echo \"linux checksum error\";"		\
		"fw_recovery; fi;" 	\
		"ledtoggle;"                                            \
		"iminfo $rootfs_addr_for_fw_checking;"					\
		"if test $? -ne 0; then echo \"rootfs checksum error\";"		\
		"fw_recovery; fi;" 	\
		"ledtoggle;"                                            \
		"lcd_print Failed!\0"					\
	"kernel_select_1="						\
		"setenv nand_pt_addr_kernel ${nand_pt_addr_kernel_1};"	\
		"setenv kernel_selected 1\0"				\
	"kernel_select_2="						\
		"setenv nand_pt_desc_kernel ${nand_pt_desc_kernel_2};"	\
		"setenv nand_pt_addr_kernel ${nand_pt_addr_kernel_2};"	\
		"setenv nand_pt_desc_kernel_2 ${nand_pt_desc_kernel_1};"\
		"setenv nand_pt_addr_kernel_2 ${nand_pt_addr_kernel_1};"\
		"setenv nand_pt_desc_kernel_1 ${nand_pt_desc_kernel};"	\
		"setenv nand_pt_addr_kernel_1 ${nand_pt_addr_kernel};"	\
		"saveenv;"						\
		"setenv kernel_selected 2\0"				\
	"kernel_rename_1="						\
		"editenv nand_pt_desc_kernel_1;"			\
		"saveenv\0"						\
	"kernel_rename_2="						\
		"editenv nand_pt_desc_kernel_2;"			\
		"saveenv\0"						\
	"kernel_select="						\
		"setenv kernel_selected 0;"				\
		"setenv bootmenu_0 ${nand_pt_desc_kernel_1}=run kernel_select_1;"	\
		"setenv bootmenu_1 ${nand_pt_desc_kernel_2}=run kernel_select_2;"	\
		"bootmenu\0"						\
	"bootnandmulti="						\
		"run kernel_select;"					\
		"if test $kernel_selected -ne 0; then run bootnand; fi\0"	\
	"hdroot="							\
		"/dev/sda1\0"						\
	"rootargsnand="							\
		"setenv rootargs " CONFIG_AL_ROOTARGS_NAND "\0"		\
	"rootargsnfs="							\
		"setenv rootargs " CONFIG_AL_ROOTARGS_NFS "\0"		\
	"rootargshd="							\
		"setenv rootargs " CONFIG_AL_ROOTARGS_HD "\0"		\
	"bootargsnand="							\
		"run rootargsnand; setenv bootargs $rootargs " CONFIG_AL_BOOTARGS_COMMON " $bootargsextra; printenv bootargs\0"	\
	"bootargsnfs="							\
		"run rootargsnfs; setenv bootargs $rootargs " CONFIG_AL_BOOTARGS_COMMON " $bootargsextra; printenv bootargs\0"	\
	"bootargshd="							\
		"run rootargshd; setenv bootargs $rootargs " CONFIG_AL_BOOTARGS_COMMON " $bootargsextra; printenv bootargs\0"	\
	"boottftp="							\
		"lcd_print \"Loading OS...\";"				\
		"tftpboot $loadaddr ${tftpdir}${dt_filename};"		\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"if test ${dt_is_from_toc} != 1; then; else "		\
			"flash_contents_obj_read_mem $loadaddr_dt $loadaddr;"	\
			"if test $? -ne 0; then run fail; exit; fi;"	\
		"fi;"							\
		"tftpboot $loadaddr ${tftpdir}uImage;"			\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"fdt addr $loadaddr_dt;"				\
		"bootm $loadaddr - $fdtaddr;"				\
		"run fail; exit\0"					\
	"ext4dir=boot/\0"						\
	"ext4dev=0\0"							\
	"ext4part=1\0"							\
	"bootext4="							\
		"lcd_print \"Loading OS...\";"				\
		"scsi init; "						\
		"ext4load scsi ${ext4dev}:${ext4part} $loadaddr ${ext4dir}${dt_filename};"	\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"if test ${dt_is_from_toc} != 1; then; else "		\
			"flash_contents_obj_read_mem $loadaddr_dt $loadaddr;"	\
			"if test $? -ne 0; then run fail; exit; fi;"	\
		"fi;"							\
		"ext4load scsi ${ext4dev}:${ext4part} $loadaddr ${ext4dir}uImage;"	\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"fdt addr $loadaddr_dt;"				\
		"bootm $loadaddr - $fdtaddr;"				\
		"run fail; exit\0"					\
	"bootupdy="							\
		"is_nand_boot; if test $? -eq 0; then run bootupdspiy;"	\
		"else; run bootupdnandy; fi\0"				\
	"bootupd="							\
		"is_nand_boot; if test $? -eq 0; then run bootupdspi; "	\
		"else; run bootupdnand; fi\0"				\
	"bootupdspi="							\
		"lcd_print \"Updating al-boot\" \"to SPI\"; "		\
		"tftpboot ${loadaddr} ${tftpdir}boot.img; "		\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"sf probe; "						\
		"sf erase 0 +${filesize}; "				\
		"sf write ${loadaddr} 0 ${filesize}; "			\
		"echo bootupd done;"					\
		"echo Notice: Changes in default environment "		\
		"variables will only take effect once the;"		\
		"echo environment variables are deleted from "		\
		"flash using the 'delenv' script;"			\
		"lcd_print \"Done\"\0"					\
	"bootupdspiy="							\
		"lcd_print \"Updating al-boot\" \"to SPI\"; "		\
		"echo >> Use YModem to upload the boot image binary...;"	\
		"loady ${loadaddr};"					\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"sf probe; "						\
		"sf erase 0 +${filesize}; "				\
		"sf write ${loadaddr} 0 ${filesize}; "			\
		"echo bootupd done;"					\
		"echo Notice: Changes in default environment "		\
		"variables will only take effect once the;"		\
		"echo environment variables are deleted from "		\
		"flash using the 'delenv' script;"			\
		"lcd_print \"Done\"\0"					\
	"bootupdnand="							\
		"lcd_print \"Updating al-boot\" \"to NAND\"; "		\
		"tftpboot $loadaddr ${tftpdir}boot.img; "		\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"nand erase.spread $nand_pt_addr_al_boot $filesize; "	\
		"nand write $loadaddr $nand_pt_addr_al_boot $filesize; "	\
		"lcd_print \"Done\"\0"				\
	"bootupdnandy="							\
		"lcd_print \"Updating al-boot\" \"to NAND\"; "		\
		"echo >> Use YModem to upload the boot image binary...;"	\
		"loady $loadaddr;"					\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"nand erase.spread $nand_pt_addr_al_boot $filesize; "	\
		"nand write $loadaddr $nand_pt_addr_al_boot $filesize; "\
		"lcd_print \"Done\"\0"					\
	"dtupd="							\
		"is_nand_boot; if test $? -eq 0; then run dtupdspi; "	\
		"else; run dtupdnand; fi\0"				\
	"dtupdy="							\
		"is_nand_boot; if test $? -eq 0; then run dtupdspiy; "	\
		"else; run dtupdnandy; fi\0"				\
	"dtupdspi="							\
		"lcd_print \"Updating DT\" \"to SPI\"; "		\
		"tftpboot $loadaddr_dt ${tftpdir}${dt_filename};"	\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"if test ${dt_is_from_toc} != 1; then; else "		\
			"flash_contents_obj_read_mem $loadaddr $loadaddr_dt;"	\
			"if test $? -ne 0; then run fail; exit; fi;"	\
		"fi;"							\
		"sf probe; "						\
		"sf erase ${dt_location} +${filesize}; "		\
		"sf write ${loadaddr_dt} ${dt_location} ${filesize};"	\
		"echo dtupd done;"					\
		"lcd_print \"Done\"\0"					\
	"dtupdspiy="							\
		"lcd_print \"Updating DT\" \"to SPI\"; "		\
		"echo >> Use YModem to upload the device tree binary...;"	\
		"loady $loadaddr_dt;"				\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"if test ${dt_is_from_toc} != 1; then; else "		\
			"flash_contents_obj_read_mem $loadaddr $loadaddr_dt;"	\
			"if test $? -ne 0; then run fail; exit; fi;"	\
		"fi;"							\
		"sf probe; "						\
		"sf erase ${dt_location} +${filesize}; "		\
		"sf write ${loadaddr_dt} ${dt_location} ${filesize}; "\
		"echo dtupd done;"					\
		"lcd_print \"Done\"\0"					\
	"dtupdnand="							\
		"lcd_print \"Updating DT\" \"to NAND\"; "		\
		"tftpboot $loadaddr_dt ${tftpdir}${dt_filename};"	\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"if test ${dt_is_from_toc} != 1; then; else "		\
			"flash_contents_obj_read_mem $loadaddr $loadaddr_dt;"	\
			"if test $? -ne 0; then run fail; exit; fi;"	\
		"fi;"							\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"nand erase.spread ${dt_location} 10000; "		\
		"nand write ${loadaddr_dt} ${dt_location} 10000; "	\
		"lcd_print \"Done\"\0"					\
	"dtupdnandy="							\
		"lcd_print \"Updating DT\" \"to NAND\"; "		\
		"echo >> Use YModem to upload the device tree binary...;"	\
		"loady $loadaddr_dt;"					\
		"if test $? -ne 0; then run fail; exit; fi;"		\
		"if test ${dt_is_from_toc} != 1; then; else "		\
			"flash_contents_obj_read_mem $loadaddr $loadaddr_dt;"	\
			"if test $? -ne 0; then run fail; exit; fi;"	\
		"fi;"							\
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"nand erase.spread ${dt_location} 10000; "		\
		"nand write ${loadaddr_dt} ${dt_location} 10000; "	\
		"lcd_print \"Done\"\0"					\
	"delenv="							\
		"is_nand_boot; if test $? -eq 0; then run delenvspi; "	\
		"else; run delenvnand; fi\0"				\
	"delenvspi="							\
		"lcd_print \"Deleting env...\"; "			\
		"sf probe; "						\
		"sf erase ${env_offset} +2000;"				\
		"if test -n ${env_offset_redund}; then "		\
			"sf erase ${env_offset_redund} +2000;"		\
		"fi;"							\
		"lcd_print \"Done\"\0"					\
	"delenvnand="							\
		"lcd_print \"Deleting env...\"; "			\
		"nand erase ${env_offset} 2000; "			\
		"if test -n ${env_offset_redund}; then "		\
			"nand erase ${env_offset_redund} 2000; "	\
		"fi;"							\
		"lcd_print \"Done\"\0"					\
	"skip_eth_halt=0\0"						\
	"loadaddr_payload=0x08000004\0"					\
	"loadaddr_dt=0x07000000\0"					\
	"loadaddr_rootfs_chk=0x07000000\0"				\
	"eepromupd="							\
		"confirm_msg \"Perform EEPROM update? [y/n] \";"	\
		"if test $? -ne 0; then exit; fi;"			\
		"tftpboot ${tftpdir}eeprom.bin;"			\
		"if test $? -ne 0; then exit; fi;"			\
		"i2c probe ${pld_i2c_addr};"				\
		"if test $? -ne 0; then exit; fi;"			\
		"i2c write $fileaddr ${pld_i2c_addr} 0.2 $filesize;"	\
		"if test $? -ne 0; then exit;fi;"			\
		"echo eepromupd done\0"					\
	"eepromupdy="							\
		"confirm_msg \"Perform EEPROM update? [y/n] \";"	\
		"if test $? -ne 0; then exit; fi;"			\
		"echo >> Use YModem to upload the EEPROM binary...;"	\
		"loady $loadaddr;"					\
		"if test $? -ne 0; then exit; fi;"			\
		"i2c probe ${pld_i2c_addr};"				\
		"if test $? -ne 0; then exit; fi;"			\
		"i2c write $loadaddr ${pld_i2c_addr} 0.2 $filesize;"	\
		"if test $? -ne 0; then exit;fi;"			\
		"echo eepromupdy done\0"				\
	"iocc_force=1\0"						\
	"iocc_force_val=1\0"						\
	CONFIG_CVOS_ENV_COMMANDS					\
	CONFIG_AL_NAND_ENV_COMMANDS					\
	""

#define CONFIG_SYS_THUMB_BUILD
#define CONFIG_IPADDR    192.168.1.1
#define CONFIG_NETMASK   255.255.255.0
#define CONFIG_SERVERIP  192.168.1.10
#define CONFIG_HW29765235P0P512P1024P4X4P4X4

#define DNI_NAND
#define CONFIG_SYS_FLASH_SECTOR_SIZE 0x20000

#define FIRMWARE_RECOVER_FROM_TFTP_SERVER 1
#define CONFIG_SYS_IMAGE_LEN   0x09E00000
#define CONFIG_SYS_IMAGE_BASE_ADDR  0x00580000
#define CONFIG_SYS_IMAGE_ADDR_BEGIN (CONFIG_SYS_IMAGE_BASE_ADDR)
#define CONFIG_SYS_IMAGE_ADDR_END   (CONFIG_SYS_IMAGE_BASE_ADDR + CONFIG_SYS_IMAGE_LEN)
#define CONFIG_SYS_STRING_TABLE_LEN                0x32000
#define CONFIG_SYS_STRING_TABLE_NUMBER 17
#define CONFIG_SYS_STRING_TABLE_TOTAL_LEN          0x380000
#define CONFIG_SYS_FLASH_CONFIG_BASE  0x001fdc0000
#define CONFIG_SYS_FLASH_CONFIG_PARTITION_SIZE  0x120000

#define CONFIG_SYS_NMRP                1
#define CONFIG_CMD_NMRP                1

#define CONFIG_EXTRA_FOUR_BYTE_KERNEL_LENGTH 1
#define CONFIG_DISPLAY_BOARDINFO

/*
 * Manufacturing Data
 */
#define CONFIG_CMD_BOARD_PARAMETERS

#define BOARDCAL                0x300000
#define BOARDCAL_LEN            0x140000

#define CONFIG_TRIPLE_MAC_ADDRESS 1

#define LAN_MAC_OFFSET          0x00
#define WAN_MAC_OFFSET          0x06
#define WLAN_MAC_OFFSET         0x0c

#define WPSPIN_OFFSET           0x12
#define WPSPIN_LENGTH           8

/* 12(lan/wan) + 6(wlan5g) + 8(wpspin) = 26 (0x1a)*/
#define SERIAL_NUMBER_OFFSET        0x1a
#define SERIAL_NUMBER_LENGTH        13

#define REGION_NUMBER_OFFSET        0x27
#define REGION_NUMBER_LENGTH        2

#define BOARD_HW_ID_OFFSET          (REGION_NUMBER_OFFSET + REGION_NUMBER_LENGTH)
#define BOARD_HW_ID_LENGTH          34

#define BOARD_MODEL_ID_OFFSET       (BOARD_HW_ID_OFFSET + BOARD_HW_ID_LENGTH)
#define BOARD_MODEL_ID_LENGTH       16

#define BOARD_SSID_OFFSET           (BOARD_MODEL_ID_OFFSET + BOARD_MODEL_ID_LENGTH)
#define BOARD_SSID_LENGTH           32

#define BOARD_PASSPHRASE_OFFSET     (BOARD_SSID_OFFSET + BOARD_SSID_LENGTH)
#define BOARD_PASSPHRASE_LENGTH     64

#define BLUETOOTH_MAC_OFFSET        (BOARD_PASSPHRASE_OFFSET + BOARD_PASSPHRASE_LENGTH)
#define BLUETOOTH_MAC_LENGTH        6

#define SFP_MAC_OFFSET             (BLUETOOTH_MAC_OFFSET + BLUETOOTH_MAC_LENGTH)
#define SFP_MAC_LENGTH             6

#define WLAN11AD_MAC_OFFSET             (SFP_MAC_OFFSET + SFP_MAC_LENGTH)
#define WLAN11AD_MAC_LENGTH             6

#define CONFIG_USE_COMMON_BOARD_ID_IMPLEMENTATION 1
#define ETH_B_RST_GPIO  45  /* reset QCA8337-B (Output pin/Active HIGH) */
#define ETH_A_RST_GPIO  46  /* reset QCA8337-A (Output pin/Active HIGH) */

#define CHECK_DNI_FIRMWARE_INTEGRITY
#define CHECK_DNI_FIRMWARE_ROOTFS_INTEGRITY

#define	WIFI_ON_OFF_BUTTON	5
#define	POWER_LED			22
#define	INTERNET_LED		23
#define	WIFI_LED			29
#define	DAC_LED				30
#define	RESET_BUTTON		31
#define	WPS_BUTTON			32
#define	LED_ON_OFF_BUTTON	35
#define	USB1_LED			36
#define	USB2_LED			37
#define	WPS_LED				39
#define	STATUS_LED

#define CONFIG_MISC_INIT_R

#define WORKAROUND_ANNAPURNA_AL314_GMAC_NMRP_HANG 1

#define CONFIG_SYS_STRING_TABLE_LEN                0x32000    /* Each string table takes 200KB in UBI volume to save */
#define CONFIG_SYS_STRING_TABLE_NUMBER 10
#define CONFIG_SYS_STRING_TABLE_TOTAL_LEN          0x1f4000   /* Totally allocate 2000KB in UBI volume to save all string tables */
#define CONFIG_SYS_STRING_TABLE_PARTITION_NAME     "language"         /* Partition which contains UBI volume below */
#define CONFIG_SYS_STRING_TABLE_BASE_ADDR          0x1fa40000	

#endif
