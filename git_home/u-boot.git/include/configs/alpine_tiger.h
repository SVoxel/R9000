#ifndef __AL_ALPINE_DB_H
#define __AL_ALPINE_DB_H

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

#define CONFIG_SYS_THUMB_BUILD

#endif
