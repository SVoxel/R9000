/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Manage the QCA8511 Music Switch.
 *
 * All definitions in this file are operating system independent!
 */

#include <common.h>
#include "athrs17_phy.h"
#include "qca8511.h"
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
#include "../../../drivers/net/ipq/ipq_gmac_hw29764958p0p128p512p3x3p4x4.h"
#endif
#undef DEBUG
#ifdef DEBUG
#define dbg(format, arg...) dbg("DEBUG: " format "\n", ## arg)
#else
#define dbg(format, arg...) do {} while(0)
#endif /* DEBUG */


static uint32_t
qca8511_pp_reg_read(uint32_t reg_addr)
{
	uint32_t reg_word_addr;
	uint32_t phy_addr, tmp_val, reg_val;
	uint16_t phy_val;
	uint8_t phy_reg;

	/*
	 * Change reg_addr to 16-bit word address, 32-bit aligned.
	 */
	reg_word_addr = (reg_addr & 0xfffffffc) ;
	dbg("WJL %s: 0-reg_addr=0x%08x, reg_word_addr=0x%08x.\n\n",
				__func__, reg_addr, reg_word_addr);

	/*
	 * configure register high address;
	 */
	phy_addr = 0x18 | (reg_word_addr >> 29);
	phy_reg = (reg_word_addr & 0x1f000000) >> 24;
	/*
	 *  Bit23-8 of reg address
	 */
	phy_val = (uint16_t) ((reg_word_addr >> 8) & 0xffff);
	phy_reg_write(0, phy_addr, phy_reg, phy_val);
	dbg("WJL %s: 1-w.phy_addr=0x%04x, phy_reg=0x%04x,"
			"phy_val=0x%04x.\n\n",
		__func__, phy_addr, phy_reg, phy_val);

	/*
	 * For some registers such as MIBs, since it is read/clear,
	 * we should read the lower 16-bit register then the higher one
	 */

	/*
	 * Read register in lower address
	 * Bit7-5 of reg address
	 */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7);
	/*
	 * Bit4-0 of reg address.
	 */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);
	reg_val = (uint32_t) phy_reg_read(0, phy_addr, phy_reg);
	dbg("WJL %s: 2-r.phy_addr=0x%04x, phy_reg=0x%04x,"
			"phy_val=0x%04x, reg_val=0x%04x.\n\n",
			__func__, phy_addr, phy_reg, phy_val, reg_val);

	/*
	 * Read register in higher address
	 * bit7-5 of reg address
	 */

	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7);
	/*
	 * Bit4-0 of reg address.
	 */
	phy_reg = (uint8_t) ((reg_word_addr & 0x1f) | 0x2);
	tmp_val = (uint32_t) phy_reg_read(0, phy_addr, phy_reg);
	reg_val |= (tmp_val << 16);

	dbg("WJL %s: 3-r.phy_addr=0x%04x, phy_reg=0x%04x,"
			"phy_val=0x%04x, reg_val=0x%04x.\n\n",
			__func__, phy_addr, phy_reg, phy_val, reg_val);


	return reg_val;
}

static void qca8511_pp_reg_write(uint32_t reg_addr, uint32_t reg_val)
{
	uint32_t reg_word_addr;
	uint32_t phy_addr;
	uint16_t phy_val;
	uint8_t phy_reg;

	/*
	 * change reg_addr to 16-bit word address,
	 * 32-bit aligned
	 */
	reg_word_addr = (reg_addr & 0xfffffffc);
	dbg("WJL %s: 0-reg_addr=0x%08x, reg_word_addr=0x%08x.\n\n",
				__func__, reg_addr, reg_word_addr);

	/*
	 * configure register high address
	 */
	phy_addr = 0x18 | (reg_word_addr >> 29);
	phy_reg = (reg_word_addr & 0x1f000000) >> 24;
	/*
	 * Bit23-8 of reg address.
	 */
	phy_val = (uint16_t) ((reg_word_addr >> 8) & 0xffff);
	phy_reg_write(0, phy_addr, phy_reg, phy_val);
	dbg("WJL %s: 1-w.phy_addr=0x%04x, phy_reg=0x%04x,"
			"phy_val=0x%04x.\n\n",
			__func__, phy_addr, phy_reg, phy_val);
	/*
	 * For some registers such as ARL and VLAN, since they include BUSY bit
	 * in lower address, we should write the higher 16-bit register then the
	 * lower one
	 */

	/*
	 * write register in higher address
	 *  bit7-5 of reg address
	 */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7);
	/*
	 *  bit4-0 of reg address
	 */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);
	/*
	 * lowest 16bit data
	 */
	phy_val = (uint16_t) (reg_val & 0xffff);
	phy_reg_write(0, phy_addr, phy_reg, phy_val);

	dbg("WJL %s: 2-w.phy_addr=0x%04x, phy_reg=0x%04x,"
			"phy_val=0x%04x, reg_val=0x%04x.\n\n",
			__func__, phy_addr, phy_reg, phy_val, reg_val);

	/*
	 * write register in lower address
	 * bit7-5 of reg address
	 */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7);
	/*
	 * Bit4-0 of reg address
	 */
	phy_reg = (uint8_t) ((reg_word_addr & 0x1f) | 0x2);
	/*
	 * Highest 16bit data
	 */
	phy_val = (uint16_t) ((reg_val >> 16) & 0xffff);
	phy_reg_write(0, phy_addr, phy_reg, phy_val);

	dbg("WJL %s: 3-w.phy_addr=0x%04x, phy_reg=0x%04x,"
			"phy_val=0x%04x, reg_val=0x%04x.\n\n",
			__func__, phy_addr, phy_reg, phy_val, reg_val);

}

static uint32_t qca8511_pp_phy_reg_read(uint32_t phy_sel,
			uint32_t phy_addr, uint8_t reg_addr)
{
	uint32_t reg_val;
	/*
	 * B31,MDIO_BUSY
	 */
	reg_val = (1 << 31);
	/*
	 * B27, MDIO_CMD 0 = Write;1 = Read
	 */
	reg_val |= (1 << 27);
	/*
	 * B29:28, PHY_SEL MDIO channel for MDIO master.
	 * It is used to specify PHY group that to be operated.
	 */
	reg_val |= ((phy_sel & 0x3) << 28);
	/*
	 * B25:21 R/W 0 PHY_ADDR PHY address.
	 */
	reg_val |= ((phy_addr & 0x1f) << 21);
	/*
	 *  b20:16 R/W 0 REG_ADDR PHY register address.
	 */
	reg_val |= ((reg_addr & 0x1f) << 16);

	dbg("WJL %s: 1. phy_sel=0x%x, phy_addr=0x%x,"
			"reg_addr=0x%x, reg_val=0x%08x.\n",
		__func__, phy_sel, phy_addr,reg_addr, reg_val);

	qca8511_pp_reg_write(0x15004, reg_val);

	udelay(10);

	reg_val = qca8511_pp_reg_read(0x15004);

	dbg("WJL %s: 2. phy_sel=0x%x, phy_addr=0x%x,"
			"reg_addr=0x%x, reg_val=0x%08x.\n",
		__func__, phy_sel, phy_addr,reg_addr, reg_val);


	return reg_val;
}

static void qca8511_pp_phy_reg_write(uint32_t phy_sel, uint32_t phy_addr,
				uint32_t reg_addr, uint32_t reg_data)
{
	uint32_t reg_val;

	/* B31,MDIO_BUSY */
	reg_val = (1 << 31);

	/*
	 *  b27, MDIO_CMD 0 = Write; 	1 = Read;
	 */
	reg_val |= (0 << 27);
	/*
	 * B29:28, PHY_SEL MDIO channel for MDIO master.
	 * It is used to specify PHY group that to be operated.
	 */
	reg_val |= ((phy_sel & 0x3) << 28);
	/*
	 * B25:21 R/W 0 PHY_ADDR PHY address
	 */
	reg_val	|= ((phy_addr & 0x1f) << 21);
	/*
	 *  B20:16 R/W 0 REG_ADDR PHY register address
	 */
	reg_val |= ((reg_addr & 0x1f) << 16);

	/*
	 * b15:0 R/WW 0 MDIO_DATA When write, these bits are data written
	 * to PHY register. When read, these bits are data read
	 * out from PHY register.
	 */
	reg_val |= (reg_data & 0xffff);

	dbg("WJL %s: 1. phy_sel=0x%x, phy_addr=0x%x,"
			"reg_addr=0x%x, reg_val=0x%08x.\n",
		__func__, phy_sel, phy_addr,reg_addr, reg_val);

	qca8511_pp_reg_write(0x15004, reg_val);

	udelay(10);
	/*
	 * if b31-MDIO_BUSY is reset to 0?
	 */
	reg_val = qca8511_pp_reg_read(0x15004);

	dbg("WJL %s: 2. phy_sel=0x%x, phy_addr=0x%x,"
			"reg_addr=0x%x, reg_val=0x%08x.\n",
		__func__, phy_sel, phy_addr,reg_addr, reg_val);


	return ;
}


static int do_qca8511_pp_reg_read( cmd_tbl_t *cmdtp, int flag,
				int argc, char * const argv[])
{
	ulong addr, readval;
	int size;

	if ((argc < 2) || (argc > 3))
		return CMD_RET_USAGE;

	/*
	 * Check for size specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

	/*
	 * Address is specified since argc > 1
	 */
	addr = simple_strtoul(argv[1], NULL, 16);

	readval = qca8511_pp_reg_read(addr);

	printf("WJL %s: addr=0x%08x, readval=0x%08x.\n\n",
			__func__, addr, readval);

	return 0;
}

static int do_qca8511_pp_reg_write( cmd_tbl_t *cmdtp, int flag,
				int argc, char * const argv[])
{
	ulong addr, writeval, count;
	int size;

	if ((argc < 3) || (argc > 4))
		return CMD_RET_USAGE;

	/*
	 * Check for size specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

	/*
	 * Address is specified since argc > 1
	 */
	addr = simple_strtoul(argv[1], NULL, 16);

	/*
	 * Get the value to write.
	 */
	writeval = simple_strtoul(argv[2], NULL, 16);

	if (argc == 4)
		count = simple_strtoul(argv[3], NULL, 16);
	else
		count = 1;

	printf("WJL %s: addr=0x%08x, writeval=0x%08x.\n\n",
			__func__,addr, writeval);

	qca8511_pp_reg_write(addr, writeval);
	dbg("\n");
	return 0;
}

static int do_qca8511_pp_phy_reg_read( cmd_tbl_t *cmdtp, int flag,
					int argc, char * const argv[])
{
	ulong phy_sel, phy_addr, reg_addr, reg_val;
	int size;

	if (argc != 4)
		return CMD_RET_USAGE;

	/*
	 *  Check for size specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

	/*
	 * Address is specified since argc > 1
	 */
	phy_sel = simple_strtoul(argv[1], NULL, 16);
	phy_addr = simple_strtoul(argv[2], NULL, 16);
	reg_addr = simple_strtoul(argv[3], NULL, 16);

	reg_val = qca8511_pp_phy_reg_read(phy_sel, phy_addr, reg_addr);

	printf("WJL %s: phy_sel=0x%x, phy_addr=0x%x, reg_addr=0x%x,"
			"reg_val=0x%08x, size=%d.\n\n",
		__func__, phy_sel, phy_addr, reg_addr, reg_val, size);

	return 0;
}

static int do_qca8511_pp_phy_reg_write( cmd_tbl_t *cmdtp,
			int flag, int argc, char * const argv[])
{
	ulong phy_sel, phy_addr, reg_addr, reg_data;
	int size;

	if (argc != 5)
		return CMD_RET_USAGE;

	/*
	 * Check for size specification.
	 */
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

	/*
	 * Address is specified since argc > 1
	 */
	phy_sel = simple_strtoul(argv[1], NULL, 16);
	phy_addr = simple_strtoul(argv[2], NULL, 16);
	reg_addr = simple_strtoul(argv[3], NULL, 16);
	reg_data = simple_strtoul(argv[4], NULL, 16);

	printf("WJL %s: phy_sel=0x%x, phy_addr=0x%x, reg_addr=0x%x,"
			"reg_data=0x%08x, size=%d.\n",
		__func__, phy_sel, phy_addr, reg_addr, reg_data, size);

	qca8511_pp_phy_reg_write(phy_sel, phy_addr, reg_addr, reg_data);
	dbg("\n");
	return 0;
}

/*********************************************************************
 *
 * FUNCTION DESCRIPTION: This function invokes RGMII,
 * 			SGMII switch init routines.
 * INPUT : ipq_gmac_board_cfg_t *
 * OUTPUT: NONE
 *
**********************************************************************/
int ipq_qca8511_init(ipq_gmac_board_cfg_t *gmac_cfg)
{
	int ret;
	uint i;
	qca8511_pp_reg_write(QCA8511_QSGMII_1_CTRL(QSGMII_1_CTRL0),
			QSGMII_1_CH0_DUPLEX_MODE |
			QSGMII_1_CH0_LINK |
			QSGMII_1_CH0_SPEED_MODE(FORCE_1000) |
			QSGMII_1_CH0_MR_AN_EN |
			QSGMII_1_RSVD16 |
			QSGMII_1_CH0_FORCED_MODE |
			QSGMII_1_CH_MODE_CTRL(QSGMII_MAC)|
			QSGMII_1_CH0_PAUSE_SG_TX_EN |
			QSGMII_1_CH0_ASYM_PAUSE |
			QSGMII_1_CH0_PAUSE |
			QSGMII_1_RSVD30 |
			QSGMII_1_RSVD31);

	qca8511_pp_reg_write(QCA8511_QSGMII_2_CTRL(QSGMII_2_CTRL0),
			QSGMII_2_CH1_DUPLEX_MODE |
			QSGMII_2_CH1_LINK |
			QSGMII_2_CH1_SPEED_MODE(FORCE_1000) |
			QSGMII_2_CH1_MR_AN_EN |
			QSGMII_2_RSVD16 |
			QSGMII_2_CH1_FORCED_MODE |
			QSGMII_2_CH_RSVD(2) |
			QSGMII_2_CH1_PAUSE_SG_TX_EN |
			QSGMII_2_CH1_ASYM_PAUSE |
			QSGMII_2_CH1_PAUSE |
			QSGMII_2_RSVD30 |
			QSGMII_2_RSVD31);

	qca8511_pp_reg_write(QCA8511_QSGMII_3_CTRL(QSGMII_3_CTRL0),
			QSGMII_3_CH2_DUPLEX_MODE |
			QSGMII_3_CH2_LINK |
			QSGMII_3_CH2_SPEED_MODE(FORCE_1000) |
			QSGMII_3_CH2_MR_AN_EN |
			QSGMII_3_RSVD16 |
			QSGMII_3_CH2_FORCED_MODE |
			QSGMII_2_CH_RSVD(2) |
			QSGMII_3_CH2_PAUSE_SG_TX_EN |
			QSGMII_3_CH2_ASYM_PAUSE |
			QSGMII_3_CH2_PAUSE |
			QSGMII_2_RSVD30 |
			QSGMII_2_RSVD31);

	qca8511_pp_reg_write(QCA8511_QSGMII_4_CTRL(QSGMII_3_CTRL0),
			QSGMII_4_CH3_DUPLEX_MODE |
			QSGMII_4_CH3_LINK |
			QSGMII_4_CH3_SPEED_MODE(FORCE_1000) |
			QSGMII_4_CH3_MR_AN_EN |
			QSGMII_4_RSVD16 |
			QSGMII_4_CH3_FORCED_MODE |
			QSGMII_4_CH_RSVD(2) |
			QSGMII_4_CH3_PAUSE_SG_TX_EN |
			QSGMII_4_CH3_ASYM_PAUSE |
			QSGMII_4_CH3_PAUSE |
			QSGMII_4_RSVD30 |
			QSGMII_4_RSVD31);
	/*
	 * Configure 8511 Ports
	 */

	for (i = STATUS_PORT1; i < STATUS_PORT5; i++) {
		qca8511_pp_reg_write(QCA8511_PORT_STATUS_CFG(i),
				QCA8511_PORT_CFG_SPEED(FORCE_1000) |
				QCA8511_PORT_CFG_TX_MAC_EN |
				QCA8511_PORT_CFG_RX_MAC_EN |
				QCA8511_PORT_CFG_DUPLEX_MODE);
	}

	qca8511_pp_reg_write(QCA8511_GLOBAL_CTRL1,
			GLOBAL_CTL1_MAC25XG_4P3G_EN |
			GLOBAL_CTL1_MAC25XG_3P125G_EN |
			GLOBAL_CTL1_MAC26SG_1P25G_EN |
			GLOBAL_CTL1_MAC27SG_3P125G_EN |
			GLOBAL_CTL1_MAC28SG_3P125G_EN |
			GLOBAL_CTL1_MAC29SG_3P125G_EN |
			GLOBAL_CTL1_RSVD22 |
			GLOBAL_CTL1_SPI1_EN |
			GLOBAL_CTL1_TO_EXT_INT_EN |
			GLOBAL_CTL1_LED_CLK_EN_CFG |
			GLOBAL_CTL1_RSVD28 |
			GLOBAL_CTL1_RSVD29 |
			GLOBAL_CTL1_TWO_WIRE_LED_EN);

	qca8511_pp_reg_write(QCA8511_XAUI_SGMII_SERDES13_CTRL0,
			SGMII_CTRL0_RSVD(3) |
			SGMII_CTRL0_XAUI_EN_PLL(3) |
			SGMII_CTRL0_XAUI_DEEMP_CH0(1) |
			SGMII_CTRL0_XAUI_DEEMP_CH2(1) |
			SGMII_CTRL0_XAUI_DEEMP_CH3(1) |
			SGMII_CTRL0_XAUI_EN_SD(0xf));

	qca8511_pp_reg_write(QCA8511_XAUI_SGMII_SERDES13_CTRL1,
			SGMII_CTRL1_RSVD4(3) |
			SGMII_CTRL1_XAUI_EN_RX(0xf) |
			SGMII_CTRL1_XAUI_EN_TX(0xf) |
			SGMII_CTRL1_XAUI_DEEMP_CH1(1) |
			SGMII_CTRL1_XAUI_REG(9) |
			SGMII_CTRL1_RSVD23(1) |
			SGMII_CTRL1_RSVD25(2));

	printf("QCA8511 Init done....\n");
	return 0;
}

U_BOOT_CMD(
	mprd,	3,	1,	do_qca8511_pp_reg_read,
	"qca8511 packet processor register display",
	"[.b, .w, .l] address [# of objects]"
);

U_BOOT_CMD(
	mprw,	4,	1,	do_qca8511_pp_reg_write,
	"qca8511 packet processor register write (fill)",
	"[.b, .w, .l] address value [count]"
);

U_BOOT_CMD(
	mphyrd,	5,	1,	do_qca8511_pp_phy_reg_read,
	"qca8511 packet processor PHY register display",
	"[.b, .w, .l] phy_sel phy_addr reg_addr [# of objects]"
);

U_BOOT_CMD(
	mphyrw,	6,	1,	do_qca8511_pp_phy_reg_write,
	"qca8511 packet processor PHY register write (fill)",
	"[.b, .w, .l] phy_sel phy_addr reg_addr reg_data [count]"
);
