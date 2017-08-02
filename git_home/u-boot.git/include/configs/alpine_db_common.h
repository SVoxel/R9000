#ifndef __AL_ALPINE_DB_COMMON_H
#define __AL_ALPINE_DB_COMMON_H

#include "al_common.h"

#include <linux/sizes.h>

#ifndef __ASSEMBLY__
unsigned int al_bootstrap_sb_clk_get(void);
unsigned int al_bootstrap_nb_clk_get(void);
unsigned int al_spi_mode_get(void);
unsigned int al_spi_baud_rate_get(void);
void al_spi_baud_rate_set(unsigned int baud_rate);
void al_nand_max_onfi_timing_mode_set(unsigned int max_onfi_timing_mode);

unsigned int al_config_env_offset_get(void);
unsigned int al_config_env_sect_size_get(void);
unsigned int al_config_env_size_redund_get(void);
unsigned int al_config_env_range_get(void);
unsigned int al_config_env_offset_redund_get(void);
unsigned int al_config_env_addr_get(void);

#endif

#define CONFIG_SYS_GENERIC_BOARD

#if 0
#define CONFIG_EP_INIT_PRINT
#endif

/*
 * Architecture
 */

/**
 * Cache coherency
 */
#define CONFIG_SYS_DCACHE_OFF

/*
 * Display CPU and Board information
 */
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOARD_LATE_INIT

#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs */

#define CONFIG_CMD_GREPENV

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(4 << 20)	/* 4MB  */

/* Annapurna Labs south bridge clock frequency */
#define CONFIG_AL_SB_CLK_FREQ		al_bootstrap_sb_clk_get()

/**
 * Serial driver configuration
 */
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL

/* NS16550 register size, byteorder */
#define CONFIG_SYS_NS16550_REG_SIZE	-4

/* Base address of UART0 */
#define CONFIG_SYS_NS16550_COM1	AL_UART_BASE(0)
#define CONFIG_SYS_NS16550_COM2	AL_UART_BASE(1)

/* Input clock to NS16550 */
#define CONFIG_SYS_NS16550_CLK	CONFIG_AL_SB_CLK_FREQ

/* use UART0 for console */
#define CONFIG_CONS_INDEX	1

/* Default baud rate */
#define CONFIG_BAUDRATE		115200

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_BAUDRATE_TABLE	{4800, 9600, 19200, 38400, 57600,\
					115200}

/* include default commands */
#include <config_cmd_default.h>

/* remove unused commands */
#undef CONFIG_CMD_FPGA		/* FPGA configuration support */

/* turn on command-line edit/hist/auto */
#define CONFIG_CMDLINE_EDITING
#define CONFIG_COMMAND_HISTORY
#define CONFIG_AUTO_COMPLETE

#define CONFIG_SYS_NO_FLASH

#ifdef CONFIG_SYS_NO_FLASH
#undef CONFIG_CMD_IMLS
#endif

#define CONFIG_ENV_SPI_BUS	0
#define CONFIG_ENV_SPI_CS	0
#define CONFIG_ENV_SPI_MAX_HZ	CONFIG_SF_DEFAULT_SPEED
#define CONFIG_ENV_SPI_MODE	al_spi_mode_get()

#define CONFIG_AL_FLASH_TOC_FIRST_OFFSET	0
#define CONFIG_AL_FLASH_TOC_SKIP_SIZE		0x20000
#define CONFIG_AL_FLASH_TOC_MAX_NUM_SKIPS	16

/* Environment */
/* all environment share same environment size */
#define CONFIG_ENV_SIZE			SZ_16K

/* NAND */
#define CONFIG_ENV_OFFSET_NAND		0x100000
#define CONFIG_ENV_SECT_SIZE_NAND	0x40000
#define CONFIG_ENV_SIZE_REDUND_NAND	CONFIG_ENV_SIZE
#define CONFIG_ENV_RANGE_NAND		CONFIG_ENV_SECT_SIZE_NAND
#define CONFIG_ENV_OFFSET_REDUND_NAND	(CONFIG_ENV_OFFSET_NAND + CONFIG_ENV_RANGE_NAND)
/* Flash */
#define CONFIG_ENV_OFFSET_FLASH		0x70000
#define CONFIG_ENV_SECT_SIZE_FLASH	SZ_128K	/* 128K(one sector) for env */
/* SPI Flash */
#define CONFIG_ENV_OFFSET_SPI_FLASH		0x100000
#define CONFIG_ENV_SECT_SIZE_SPI_FLASH		SZ_64K
#define CONFIG_ENV_SIZE_REDUND_SPI_FLASH	CONFIG_ENV_SIZE
#define CONFIG_ENV_RANGE_SPI_FLASH		CONFIG_ENV_SECT_SIZE_SPI_FLASH
#define CONFIG_ENV_OFFSET_REDUND_SPI_FLASH	(CONFIG_ENV_OFFSET_SPI_FLASH + CONFIG_ENV_SECT_SIZE_SPI_FLASH)
/* Nowhere */

#if (0)
	#define CONFIG_ENV_IS_IN_NAND		1
	#define CONFIG_ENV_OFFSET		CONFIG_ENV_OFFSET_NAND
	#define CONFIG_ENV_SECT_SIZE		CONFIG_ENV_SECT_SIZE_NAND
	#define CONFIG_ENV_SIZE_REDUND		CONFIG_ENV_SIZE_REDUND_NAND
	#define CONFIG_ENV_RANGE		CONFIG_ENV_RANGE_NAND
	#define CONFIG_ENV_OFFSET_REDUND	CONFIG_ENV_OFFSET_REDUND_NAND
#elif (0)
	#define CONFIG_ENV_IS_IN_FLASH		1
	#define CONFIG_ENV_OFFSET		CONFIG_ENV_OFFSET_FLASH
	#define CONFIG_ENV_SECT_SIZE		CONFIG_ENV_SECT_SIZE_FLASH
#elif (0)
	#define CONFIG_ENV_IS_IN_SPI_FLASH	1
	#define CONFIG_ENV_OFFSET		CONFIG_ENV_OFFSET_SPI_FLASH
	#define CONFIG_ENV_SECT_SIZE		CONFIG_ENV_SECT_SIZE_SPI_FLASH
#elif (0)
	#define CONFIG_ENV_IS_NOWHERE		1	/* Store ENV in memory only */
#else
	#define CONFIG_ENV_IS_DYNAMIC		1
		#define CONFIG_ENV_IS_IN_NAND		1
		#define CONFIG_ENV_IS_IN_SPI_FLASH	1
		#define CONFIG_ENV_IS_NOWHERE		1

	#define CONFIG_ENV_OFFSET		al_config_env_offset_get()
	#define CONFIG_ENV_SECT_SIZE		al_config_env_sect_size_get()
	#define CONFIG_ENV_SIZE_REDUND		al_config_env_size_redund_get()
	#define CONFIG_ENV_RANGE		al_config_env_range_get()
	#define CONFIG_ENV_OFFSET_REDUND	al_config_env_offset_redund_get()
#endif

#define CONFIG_ENV_VARS_UBOOT_CONFIG

#define CONFIG_AL_ETH
/*
 *  this config prevent from changing to another eth port
 *  when connection to the current one can't be establish.
 */
#define CONFIG_NET_DO_NOT_TRY_ANOTHER

#ifdef CONFIG_AL_ETH
#define AL_ETH_ENABLE_VECTOR			0xf	/* All enabled */

#define AL_ETH_PHY_ADDR_ETH0			0
#define AL_ETH_PHY_ADDR_ETH1			4
#define AL_ETH_PHY_ADDR_ETH2			0
#define AL_ETH_PHY_ADDR_ETH3			5

#define CONFIG_MII
#define CONFIG_CMD_MII
#define CONFIG_PHYLIB
#define CONFIG_PHY_ATHEROS
#define CONFIG_PHY_REALTEK
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_PING
#define CONFIG_CMD_TFTPPUT
#define CONFIG_RANDOM_MACADDR

/* Num RX buffers - must be (2^n - 1) */
#define CONFIG_SYS_RX_ETH_BUFFER		15
#endif

#define CONFIG_PCI
#ifdef CONFIG_PCI
#define CONFIG_PCI_CLEANUP
#define CONFIG_SYS_PCI_64BIT
#define CONFIG_PCI_SCAN_SHOW
#define CONFIG_CMD_PCI
/* enable pcie external devices */
#define CONFIG_AL_PCIE_0
#define CONFIG_AL_PCIE_1
#define CONFIG_AL_PCIE_2
/* #define CONFIG_CMD_AL_PCI_EXT */
#endif /* CONFIG_PCI */

/* PXE */
#define CONFIG_CMD_PXE
/* needed for pxe cmd */
#define CONFIG_MENU

#define CONFIG_LIB_RAND

#define CONFIG_SCSI_AHCI
#ifdef CONFIG_SCSI_AHCI
#define CONFIG_LIBATA
#define CONFIG_SYS_SCSI_MAX_SCSI_ID	4
#define CONFIG_SYS_SCSI_MAX_LUN		1
#define CONFIG_SYS_SCSI_MAX_DEVICE	(CONFIG_SYS_SCSI_MAX_SCSI_ID * \
					CONFIG_SYS_SCSI_MAX_LUN)
#define CONFIG_SCSI_DEV_LIST		{ PCI_VENDOR_ID_ANNAPURNALABS, \
					  PCI_DEVICE_ID_AL_SATA }
#define CONFIG_CMD_SCSI
#define CONFIG_CMD_EXT4
#define CONFIG_EFI_PARTITION
#endif /* CONFIG_SCSI_AHCI */

/* SPI driver configuration */
#define CONFIG_SPI_FLASH
#ifdef CONFIG_SPI_FLASH
#define CONFIG_AL_SPI
#define	CONFIG_CMD_SF
#define CONFIG_SPI_FLASH_ATMEL
#define CONFIG_SPI_FLASH_EON
#define CONFIG_SPI_FLASH_GIGADEVICE
#define	CONFIG_SPI_FLASH_MACRONIX
#define CONFIG_SPI_FLASH_SPANSION
#define	CONFIG_SPI_FLASH_STMICRO
#define CONFIG_SPI_FLASH_SST
#define CONFIG_SPI_FLASH_WINBOND
#define AL_SPI_CS_NUM			4
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_MODE		al_spi_mode_get()
#define CONFIG_SF_DEFAULT_SPEED		al_spi_baud_rate_get()
#define AL_SPI_BAUD_RATE_DEFAULT	37500000
#endif

/* NAND driver configuration */
#define CONFIG_AL_NAND_FLASH
#ifdef CONFIG_AL_NAND_FLASH
#define CONFIG_CMD_NAND
#define CONFIG_SYS_NAND_SELF_INIT
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_MAX_CHIPS	4
#define CONFIG_SYS_NAND_BASE		0
#define CONFIG_SYS_NAND_ONFI_DETECTION
/* #define CONFIG_AL_NAND_SIMULATE */
#define CONFIG_AL_NAND_ECC_SUPPORT
#define AL_NAND_USE_WRAPPER		1
#define AL_NAND_MAX_ONFI_TIMING_MODE_DEFAULT	1
#endif

/* Environment information, boards can override if required */
#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define CONFIG_LOADADDR		SZ_128M	/* def. location for kernel */
#define CONFIG_BOOTDELAY	2 /* -1 to disable auto boot, 0 for no delay */
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_BOOTCOMMAND	"run bootargsnand; run bootnand"
#define CONFIG_BOOTARGS		""
#define CONFIG_AL_BOOTARGS_COMMON	\
	"pci=pcie_bus_perf console=ttyS0,115200"
#define CONFIG_AL_ROOTARGS_NAND	\
	"root=ubi0:root rootfstype=ubifs ubi.mtd=3"
#define CONFIG_AL_ROOTARGS_NFS	\
	"root=/dev/nfs rw nfsroot=${serverip}:${nfsrootdir},tcp,nolock rw ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${board_name}:eth1:none"
#define CONFIG_AL_ROOTARGS_HD	\
	"root=${hdroot} rw"

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser */
#define CONFIG_SYS_PROMPT		"ALPINE_DB> "
#define CONFIG_SYS_CBSIZE		(2048) /* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		(CONFIG_SYS_CBSIZE)

#define CONFIG_CMD_MEMTEST

#define CONFIG_SYS_MEMTEST_START	SZ_32M
#define CONFIG_SYS_MEMTEST_END		SZ_1G

#define CONFIG_SYS_LOAD_ADDR		(0xA00800)	/* default */

#define CONFIG_SYS_ARCH_TIMER		1
#define CONFIG_SYS_HZ_CLOCK		(al_bootstrap_nb_clk_get() / 16)
#define CONFIG_SYS_HZ			1000

/* enable md5 checksum */
#define CONFIG_MD5
#define CONFIG_CMD_MD5SUM
#define CONFIG_MD5SUM_VERIFY

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	4

#define CONFIG_SYS_SDRAM_BASE		0x00000000
#define CONFIG_SYS_TEXT_BASE		0x00100000
#define CONFIG_SYS_INIT_SP_ADDR		0x01000000
#define CONFIG_STACKSIZE_ABORT		SZ_64K

#define CONFIG_GPIO
#ifdef CONFIG_GPIO
#define CONFIG_AL_GPIO

#define CONFIG_CMD_GPIO
#endif /* CONFIG_GPIO */

#define CONFIG_AL_I2C_PLD
/* Define CONFIG_AL_I2C_GEN/PLD according to I2C usage - General or Preloader */
#ifdef CONFIG_AL_I2C_GEN
#define CONFIG_I2C
#define CONFIG_SYS_I2C_BASE		AL_I2C_GEN_BASE
#elif defined(CONFIG_AL_I2C_PLD)
#define CONFIG_I2C
#define CONFIG_SYS_I2C_BASE		AL_I2C_PLD_BASE
#endif /* CONFIG_I2C_PLD */

#ifdef CONFIG_I2C
#define CONFIG_SYS_I2C_DW
#define CONFIG_HARD_I2C
#define CONFIG_CMD_I2C
#define IC_CLK				((CONFIG_AL_SB_CLK_FREQ) / 1000000)
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		0x08 /* dummy address */
#define CONFIG_AL_I2C

#define CONFIG_SYS_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_BUS_MAX		1
#define CONFIG_SYS_NUM_I2C_BUSES	9
#define CONFIG_SYS_I2C_MAX_HOPS		1
#define CONFIG_SYS_I2C_BUSES	{	{0, {I2C_NULL_HOP} }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 0} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 1} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 2} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 3} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 4} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 5} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 6} } }, \
					{0, {{I2C_MUX_PCA9548, 0x70, 7} } }, \
				}


#endif /* CONFIG_AL_I2C */

#define CONFIG_CMD_ENTERRCM
#define CONFIG_CMD_BOOTZ
#define CONFIG_CMD_BOOTMENU

#define CONFIG_AL_NAND_ENV_COMMANDS					\
	"nand_pt_addr_al_boot=0x0\0"					\
	"nand_pt_size_al_boot=0x00400000\0"				\
	"nand_pt_addr_kernel=0x00400000\0"				\
	"nand_pt_size_kernel=0x00c00000\0"				\
	"nand_pt_addr_fs=0x01000000\0"					\
	"nand_pt_size_fs=0x3f000000\0"					\
	"nand_pt_desc_kernel_1=Test kernel A\0"				\
	"nand_pt_addr_kernel_1=0x00400000\0"				\
	"nand_pt_desc_kernel_2=Test kernel B\0"				\
	"nand_pt_addr_kernel_2=0x00a00000\0"

#define CONFIG_EXTRA_ENV_SETTINGS					\
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
		"nand set_partition_offset $nand_pt_addr_al_boot;"	\
		"bootm $loadaddr_payload - $fdtaddr;"			\
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
	CONFIG_CVOS_ENV_COMMANDS					\
	CONFIG_AL_NAND_ENV_COMMANDS					\
	""
/* Device Tree */
#define CONFIG_OF_LIBFDT
#define CONFIG_OF_BOARD_SETUP
#define CONFIG_AL_DT_MAX_SIZE			SZ_64K

#endif /* __AL_ALPINE_DB_H */
