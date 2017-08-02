/**
 * board/annapurna-labs/alpine_db/board_cfg.h
 *
 * Annapurna Labs Alpine development board static configuration
 *
 * Copyright (C) 2013 Annapurna Labs Ltd.
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
#include <pll_init.h>
#include <gpio_board_init.h>
#include <al_hal_muio_mux.h>

/******************************************************************************
 * Board IDs
 ******************************************************************************/
#define BOARD_ID_GENERIC		0
#define BOARD_ID_ENDPOINT		1

/******************************************************************************
 * MUIO Mux
 ******************************************************************************/
static const struct al_muio_mux_if_and_arg muio_mux_ifaces[] = {
	/*{ AL_MUIO_MUX_IF_NOR_8, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_16, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_0, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_WP, 0 },*/
	{ AL_MUIO_MUX_IF_NAND_8, 0 },
	/*{ AL_MUIO_MUX_IF_NAND_16, 0 },*/
	{ AL_MUIO_MUX_IF_NAND_CS_0, 0 },
	/*{ AL_MUIO_MUX_IF_NAND_CS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_NAND_CS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_NAND_CS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_NAND_WP, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_8, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_16, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_0, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_SATA_0_LEDS, 0 },*/
	{ AL_MUIO_MUX_IF_SATA_1_LEDS, 0 },
	{ AL_MUIO_MUX_IF_ETH_LEDS, 0 },
	/*{ AL_MUIO_MUX_IF_ETH_GPIO, 0 },*/
	{ AL_MUIO_MUX_IF_UART_1, 0 },
	/*{ AL_MUIO_MUX_IF_UART_1_MODEM, 0 },*/
	/*{ AL_MUIO_MUX_IF_UART_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_UART_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_I2C_GEN, 0 },*/
	/*{ AL_MUIO_MUX_IF_ULPI_0_RST_N, 0 },*/
	/*{ AL_MUIO_MUX_IF_ULPI_1_RST_N, 0 },*/
	/*{ AL_MUIO_MUX_IF_PCI_EP_INT_A, 0 },*/
	/*{ AL_MUIO_MUX_IF_SPIM_A_SS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_SPIM_A_SS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_SPIM_A_SS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_ULPI_1_B, 0 },*/
};

static const struct al_muio_mux_if_and_arg muio_mux_ifaces_with_uart123[] = {
	/*{ AL_MUIO_MUX_IF_NOR_8, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_16, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_0, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_CS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_NOR_WP, 0 },*/
	{ AL_MUIO_MUX_IF_NAND_8, 0 },
	/*{ AL_MUIO_MUX_IF_NAND_16, 0 },*/
	{ AL_MUIO_MUX_IF_NAND_CS_0, 0 },
	/*{ AL_MUIO_MUX_IF_NAND_CS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_NAND_CS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_NAND_CS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_NAND_WP, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_8, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_16, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_0, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_SRAM_CS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_SATA_0_LEDS, 0 },*/
	/*{ AL_MUIO_MUX_IF_SATA_1_LEDS, 0 },*/
	/*{ AL_MUIO_MUX_IF_ETH_LEDS, 0 },*/
	/*{ AL_MUIO_MUX_IF_ETH_GPIO, 0 },*/
	{ AL_MUIO_MUX_IF_UART_1, 0 },
	/*{ AL_MUIO_MUX_IF_UART_1_MODEM, 0 },*/
	{ AL_MUIO_MUX_IF_UART_2, 0 },
	{ AL_MUIO_MUX_IF_UART_3, 0 },
	/*{ AL_MUIO_MUX_IF_I2C_GEN, 0 },*/
	/*{ AL_MUIO_MUX_IF_ULPI_0_RST_N, 0 },*/
	/*{ AL_MUIO_MUX_IF_ULPI_1_RST_N, 0 },*/
	/*{ AL_MUIO_MUX_IF_PCI_EP_INT_A, 0 },*/
	/*{ AL_MUIO_MUX_IF_SPIM_A_SS_1, 0 },*/
	/*{ AL_MUIO_MUX_IF_SPIM_A_SS_2, 0 },*/
	/*{ AL_MUIO_MUX_IF_SPIM_A_SS_3, 0 },*/
	/*{ AL_MUIO_MUX_IF_ULPI_1_B, 0 },*/
};

/******************************************************************************
 * GPIOs
 ******************************************************************************/
/**
 * Board GPIOs indice definitions
 */

/**
 * 0- 3x Uart module inserted
 * Input
 */
#define GPIO_INDEX_RS232_EN		0

#define GPIO_INDEX_ULPI_0_RST_N		3

#ifdef NO_BOARD_INFO_GPIO
#define GPIO_INDEX_BOARD_ID_0		21
#define GPIO_INDEX_BOARD_ID_1		22
#define GPIO_INDEX_BOARD_ID_2		23
#endif

/**
 * 0 - pCIe0 CARD is connetced
 * Input
 * Initialize core accordingly
 */
#define GPIO_INDEX_PCIE0_PRSNT		32

/**
 * 0 - pCIe1 CARD is connetced
 * Input
 * Initialize core accordingly
 */
#define GPIO_INDEX_PCIE1_PRSNT		33

/**
 * 0- indicate Group1 as SATA
 * Input
 * Initialize SERDES accordingly
 * Prioritize this over GPIO_INDEX_PCIe1_PRSNT
 */
#define GPIO_INDEX_PCIE2SATA_PRSNTN	34

/**
 * 0-SFP1 (or SFP0) inserted
 * Input (interruptable)
 */
#define GPIO_INDEX_SFP_ABSNT_0_1	35

/**
 * 0-SFP3 (or SFP2) inserted
 * Input (interruptable)
 */
#define GPIO_INDEX_SFP_ABSNT_2_3	36

/**
 * 0- OR of RGMII PHY interrupt 0- active
 * Input (interruptable)
 */
#define GPIO_INDEX_EXT_ETH_PHY_INT		37

/**
 * Debug LED 0
 * Output
 */
#define GPIO_INDEX_DBG_LED_0		1

/**
 * Debug LED 1
 * Output
 */
#define GPIO_INDEX_DBG_LED_1		2

/**
 * 1- Preload eprom can be access 0- I2c can access SFP
 * Output
 * Set to 0
 */
#define GPIO_INDEX_PRELOADEPROM_EN	5

/**
 * PCI slot Reset  0- PCI Reseted / 1- PCI active
 * Output
 * Set to active
 */
#define GPIO_INDEX_PCI_RSTN		38

/**
 * 1- SFP ON 0- SFP off
 * Output
 * Set to on
 */
#define GPIO_INDEX_SFP_ON_OFF		39

/**
 * 1 -ETH A and 1 -ETH B RGMII PHY reset
 * Output
 * Set to 0 After clocks
 */
#ifdef GPIO_ETH_PHY_A_RESET
#define GPIO_INDEX_EXT_ETH_PHY_A_RST		GPIO_ETH_PHY_A_RESET
#else
#define GPIO_INDEX_EXT_ETH_PHY_A_RST            40
#endif
#ifdef GPIO_ETH_PHY_B_RESET
#define GPIO_INDEX_EXT_ETH_PHY_B_RST		GPIO_ETH_PHY_B_RESET
#else
#define GPIO_INDEX_EXT_ETH_PHY_B_RST		41
#endif

/**
 * 1 -ETH B RGMII PHY reset
 * Output
 * Set to 0 After clocks
 */

/**
 * 1- 1.5V 0-1.35V
 * Output
 * According to SPD, in stage 2, before DDR init - start with 1.5 volts
 */
#define GPIO_INDEX_VDD_DRAM		42

/**
 * 1- VTT OFF 0- VTT ON
 * Output
 * currently don't touch
 */
#define GPIO_INDEX_VTT_OFF		43

static const struct gpio_cfg_ent gpio_cfg[] = {
	/**
	 * Inputs
	 */
	{ GPIO_INDEX_RS232_EN, 0, 0 },
#ifdef NO_BOARD_INFO_GPIO
	{ GPIO_INDEX_BOARD_ID_0, 0, 0 },
	{ GPIO_INDEX_BOARD_ID_1, 0, 0 },
	{ GPIO_INDEX_BOARD_ID_2, 0, 0 },
#endif
	{ GPIO_INDEX_PCIE0_PRSNT, 0, 0 },
	{ GPIO_INDEX_PCIE1_PRSNT, 0, 0 },
	{ GPIO_INDEX_PCIE2SATA_PRSNTN, 0, 0 },
	{ GPIO_INDEX_SFP_ABSNT_0_1, 0, 0 },
	{ GPIO_INDEX_SFP_ABSNT_2_3, 0, 0 },
	{ GPIO_INDEX_EXT_ETH_PHY_INT, 0, 0 },

	/**
	 * Outputs
	 */
	{ GPIO_INDEX_DBG_LED_0, 1, 0 },
	{ GPIO_INDEX_DBG_LED_1, 1, 0 },
	{ GPIO_INDEX_ULPI_0_RST_N, 1, 1 },
	{ GPIO_INDEX_PRELOADEPROM_EN, 1, 0 },
	{ GPIO_INDEX_PCI_RSTN, 1, 1 },
	{ GPIO_INDEX_SFP_ON_OFF, 1, 1 },
#ifdef GPIO_ETH_PHY_RESET_VALUE
	{ GPIO_INDEX_EXT_ETH_PHY_A_RST, 1, GPIO_ETH_PHY_RESET_VALUE },
	{ GPIO_INDEX_EXT_ETH_PHY_B_RST, 1, GPIO_ETH_PHY_RESET_VALUE },
#else 
	{ GPIO_INDEX_EXT_ETH_PHY_A_RST, 1, 0 },
	{ GPIO_INDEX_EXT_ETH_PHY_B_RST, 1, 0 },
#endif /* ifdef GPIO_ETH_PHY_RESET_VALUE */
	{ GPIO_INDEX_VDD_DRAM, 1, 1 },
	{ GPIO_INDEX_VTT_OFF, 1, 0 },
};

/******************************************************************************
 * PLL
 ******************************************************************************/
#define PLL_SB_CHAN_IDX_ETH0_REF_CLK_OUT	9
#define PLL_SB_CHAN_IDX_ETH1_REF_CLK_OUT	10
#define PLL_SB_CHAN_IDX_SERDES_R2L_CLK		14
#define PLL_SB_CHAN_IDX_SERDES_L2R_CLK		15

/**
 * - Set Ethernet 0/1 RGMII reference clock outputs to 25 MHz
 * - Set Serdes R2L and L2R clocks to 100MHz (currently disabled because using
 *   PLL bypass and assuming 100MHz reference clock)
 */
static const struct pll_cfg_ent pll_sb_cfg[] = {
	{ PLL_SB_CHAN_IDX_ETH0_REF_CLK_OUT, 25000 },
	{ PLL_SB_CHAN_IDX_ETH0_REF_CLK_OUT, 25000 },
/*	{ PLL_SB_CHAN_IDX_SERDES_R2L_CLK, 100000 },*/	/* R2L */
/*	{ PLL_SB_CHAN_IDX_SERDES_L2R_CLK, 100000 },*/	/* L2R */
};

