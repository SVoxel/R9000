#include <common.h>
#include <command.h>

#include "al_board.h"

/**
 * Prints a bootstrap string value using a suitable format
 *
 * @param	param_name
 *		The name of the parameter we'd like to print
 * @param	param_val
 *		The value of the parameter we'd like to print
 */
static void al_bootstrap_print_string(
		const char *param_name,
		const char *param_val)
{
	printf("%s:\t %s\n", param_name, param_val);
}

/**
 * Prints a bootstrap frequency value using a suitable format
 *
 * The input units are Hz, and the print units are MHz
 *
 * @param	param_name
 *		The name of the parameter we'd like to print
 * @param	param_val
 *		The value of the parameter we'd like to print
 */
static void al_bootstrap_print_freq(
		const char *param_name,
		uint32_t param_val)
{
	uint32_t param_val_mhz = param_val / 1000000;
	printf("%s:\t %u[MHz]\n", param_name, param_val_mhz);
}
/**
 * Enum To String functions
 *
 * The following functions provide a simple enum to string converter
 *
 * @param	val
 *		The value we'd like to parse
 *
 * @return	a string that represents the given enum
 */
static const char *al_e2s_pll_ref_clk(enum pll_ref_clk val)
{
	switch (val) {
	case PLL_REF_CLK_HSTL:
		return "HSTL clock";
		break;
	case PLL_REF_CLK_CMOS:
		return "CMOS clock";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_i2c_preload(enum i2c_preload val)
{
	switch (val) {
	case I2C_PRELOAD_ENABLED:
		return "I2C preload enabled";
		break;
	case I2C_PRELOAD_DISABLED:
		return "I2C preload disabled";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_spis_preload(enum spis_preload val)
{
	switch (val) {
	case SPIS_PRELOAD_ENABLED:
		return "SPIS preload enabled";
		break;
	case SPIS_PRELOAD_DISABLED:
		return "SPIS preload disabled";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_boot_rom(enum boot_rom val)
{
	switch (val) {
	case BOOT_ROM_ENABLED:
		return "Boot ROM enabled";
		break;
	case BOOT_ROM_DISABLED:
		return "Boot ROM disabled";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_boot_device(enum boot_device val)
{
	switch (val) {
	case BOOT_DEVICE_NAND_8BIT:
		return "8 bit NAND Flash";
		break;
	case BOOT_DEVICE_UART:
		return "UART";
		break;
	case BOOT_DEVICE_SPI_MODE_3:
		return "SPI Flash, mode 0x3";
		break;
	case BOOT_DEVICE_SPI_MODE_0:
		return "SPI Flash, mode 0x0";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_debug_mode(enum debug_mode val)
{
	switch (val) {
	case DEBUG_MODE_ENABLED:
		return "Debug mode enabled";
		break;
	case DEBUG_MODE_DISABLED:
		return "Debug mode disabled";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_cpu_exist(enum cpu_exist val)
{
	switch (val) {
	case CPU_EXIST_0:
		return "CPU_0 is active";
		break;
	case CPU_EXIST_0_1:
		return "CPU_0 & CPU_1 are active";
		break;
	case CPU_EXIST_0_1_2_3:
		return "CPU_0 & CPU_1 & CPU_2 & CPU_3 are active";
		break;
	default:
		return "Invalid value";
	}
}

static const char *al_e2s_secure_boot(enum secure_boot val)
{
	switch (val) {
	case SECURE_BOOT_ENABLED:
		return "Secure boot enabled";
		break;
	case SECURE_BOOT_DISABLED:
		return "Secure boot disabled";
		break;
	default:
		return "Invalid value";
	}
}

int do_is_nand_boot(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	struct al_bootstrap bootstrap;
	int ret;

	ret = bootstrap_get(&bootstrap);
	if (ret != 0) {
		al_err("Bootstrap parsing failed\n");
		return 0;
	}

	if (bootstrap.boot_device == BOOT_DEVICE_NAND_8BIT)
		return 1;

	return 0;
}

int
do_dev_info(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	struct al_bootstrap bootstrap;
	int ret = 0;

	printf("Reset strap readouts:\n");
	printf("--------------------\n");

	ret = bootstrap_get(&bootstrap);
	if (ret != 0) {
		al_err("Bootstrap parsing failed\n");
		return ret;
	}

	al_bootstrap_print_freq("CPU PLL freq",
			bootstrap.cpu_pll_freq);
	al_bootstrap_print_freq("NB PLL freq",
			bootstrap.ddr_pll_freq);
	al_bootstrap_print_freq("SB PLL freq",
			bootstrap.sb_pll_freq);
	al_bootstrap_print_freq("SB clock freq",
			bootstrap.sb_clk_freq);
	al_bootstrap_print_freq("PLL ref clock freq",
			bootstrap.pll_ref_clk_freq);
	al_bootstrap_print_string("PLL ref clock mode",
		al_e2s_pll_ref_clk(bootstrap.pll_ref_clk));
	al_bootstrap_print_string("I2C Preload status",
		al_e2s_i2c_preload(bootstrap.i2c_preload));
	al_bootstrap_print_string("SPIS Preload status",
		al_e2s_spis_preload(bootstrap.spis_preload));
	al_bootstrap_print_string("Boot ROM status",
		al_e2s_boot_rom(bootstrap.boot_rom));
	al_bootstrap_print_string("Boot device",
		al_e2s_boot_device(bootstrap.boot_device));
	al_bootstrap_print_string("Debug mode status",
		al_e2s_debug_mode(bootstrap.debug_mode));
	al_bootstrap_print_string("CPU exist status",
		al_e2s_cpu_exist(bootstrap.cpu_exist));
	al_bootstrap_print_string("Secure boot status",
			al_e2s_secure_boot(bootstrap.secure_boot));
	return 0;
}

U_BOOT_CMD(
	is_nand_boot, 1, 0, do_is_nand_boot,
	"Returns 1 if boot device is NAND",
	"");

U_BOOT_CMD(
	dev_info, 1, 0, do_dev_info,
	"Show the device configuration values",
	"This command prints the important device configuration values in a"
	"convinient text format.\n"
);

