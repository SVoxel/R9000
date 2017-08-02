#include <al_globals.h>
#include <al_hal_bootstrap.h>
#include "al_board.h"

/*
 * external declarations
 */
extern int env_nand_init(void);
extern int env_nand_saveenv(void);
extern int env_nand_relocate_spec(void);
extern int env_nand_readenv(void);
extern int env_sf_init(void);
extern int env_sf_saveenv(void);
extern int env_sf_relocate_spec(void);
extern int env_nowhere_init(void);
extern int env_nowhere_saveenv(void);
extern int env_nowhere_relocate_spec(void);

#if defined(CONFIG_ENV_IS_DYNAMIC)

/*
 * get configuration functions
 */
unsigned int al_config_env_offset_get(void)
{
	struct al_bootstrap bootstrap;

	if (al_globals.env_offset_valid)
		return al_globals.env_offset;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch (al_globals.bootstraps.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return CONFIG_ENV_OFFSET_NAND;
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		return CONFIG_ENV_OFFSET_SPI_FLASH;
	default:
		return 0;
	}
}

unsigned int al_config_env_sect_size_get(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch (al_globals.bootstraps.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return CONFIG_ENV_SECT_SIZE_NAND;
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		return CONFIG_ENV_SECT_SIZE_SPI_FLASH;
	default:
		return 0;
	}
}

unsigned int al_config_env_size_redund_get(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch (bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return CONFIG_ENV_SIZE_REDUND_NAND;
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		return CONFIG_ENV_SIZE_REDUND_SPI_FLASH;
	default:
		return 0;
	}
}

unsigned int al_config_env_range_get(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch (bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return CONFIG_ENV_RANGE_NAND;
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		return CONFIG_ENV_RANGE_SPI_FLASH;
	default:
		return 0;
	}

}

unsigned int al_config_env_offset_redund_get(void)
{
	struct al_bootstrap bootstrap;

	if (al_globals.env_redund_offset_valid)
		return al_globals.env_redund_offset;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch (bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return CONFIG_ENV_OFFSET_REDUND_NAND;
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		return CONFIG_ENV_OFFSET_REDUND_SPI_FLASH;
	default:
		return 0;
	}

}

/*
 * env APIs
 */
char *env_name_spec = "TO BE DEFINED";

int env_init(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch(bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		strcpy(env_name_spec, "NAND");
		return env_nand_init();
	case BOOT_DEVICE_UART:
		return env_nowhere_init();
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		strcpy(env_name_spec, "SPI Flash");
		return 0;
	default:
		return 0;
	}
}

int env_saveenv(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch(bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return env_nand_saveenv();
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		return 0;
	default:
		return 0;
	}
}

int readenv(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return 0;

	switch(bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		return env_nand_readenv();
	case BOOT_DEVICE_UART:
		return 0;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
	default:
		return 0;
	}
}

void env_relocate_spec(void)
{
	struct al_bootstrap bootstrap;

	if (bootstrap_get(&bootstrap))
		return ;

	switch(bootstrap.boot_device) {
	case BOOT_DEVICE_NAND_8BIT:
		env_nand_relocate_spec();
		break;
	case BOOT_DEVICE_UART:
		env_nowhere_relocate_spec();
		break;
	case BOOT_DEVICE_SPI_MODE_3:
	case BOOT_DEVICE_SPI_MODE_0:
		break;
	default:
		break;
	}
}

#endif

