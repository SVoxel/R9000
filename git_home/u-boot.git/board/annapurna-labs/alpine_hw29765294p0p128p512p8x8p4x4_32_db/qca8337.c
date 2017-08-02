/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.
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
 * Manage the atheros ethernet PHY.
 *
 * All definitions in this file are operating system independent!
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <miiphy.h>
#include <phy.h>
#include <asm/errno.h>

/* Normally, RGMII is al_eth1 or al_eth3 */
#define AL_ETH_RGMII_INTERFACE "al_eth1"

void phy_reg_write(int idx, uint32_t phy_addr, uint8_t phy_reg, uint16_t phy_val)
{
	miiphy_write (AL_ETH_RGMII_INTERFACE, phy_addr, phy_reg, phy_val);
}

uint32_t phy_reg_read(int idx, uint32_t phy_addr, uint8_t phy_reg)
{
	uint32_t ret_val;

	miiphy_read (AL_ETH_RGMII_INTERFACE, phy_addr, phy_reg, &ret_val);

	return ret_val;
}

uint32_t qca8337_reg_read(uint32_t reg_addr){
	uint32_t reg_word_addr;
	uint32_t phy_addr, tmp_val, reg_val;
	uint16_t phy_val;
	uint8_t phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (uint16_t)((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	phy_reg_write(0, phy_addr, phy_reg, phy_val);

	/* For some registers such as MIBs, since it is read/clear, we should */
	/* read the lower 16-bit register then the higher one */

	/* read register in lower address */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t)(reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	reg_val = (uint32_t)phy_reg_read(0, phy_addr, phy_reg);

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t)(reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	tmp_val = (uint32_t)phy_reg_read(0, phy_addr, phy_reg);
	reg_val |= (tmp_val << 16);

	return(reg_val);
}

void qca8337_reg_write(uint32_t reg_addr, uint32_t reg_val){
	uint32_t reg_word_addr;
	uint32_t phy_addr;
	uint16_t phy_val;
	uint8_t phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (uint16_t)((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	phy_reg_write(0, phy_addr, phy_reg, phy_val);

	/* For some registers such as ARL and VLAN, since they include BUSY bit */
	/* in lower address, we should write the higher 16-bit register then the */
	/* lower one */

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t)(reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (uint16_t)((reg_val >> 16) & 0xffff);
	phy_reg_write(0, phy_addr, phy_reg, phy_val);

	/* write register in lower address */
	reg_word_addr--;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t)(reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (uint16_t)(reg_val & 0xffff);
	phy_reg_write(0, phy_addr, phy_reg, phy_val);
}

int do_qca8337_init_one(void)
{
	printf("Enable P0/5 Trunking\n");
	qca8337_reg_write (0x700, 0xa1);
	qca8337_reg_write (0x704, 0xd8);

	//qca8337_reg_write (0x620, 0x4f0);
	qca8337_reg_write (0x660, 0x0014017e);
	qca8337_reg_write (0x66c, 0x0014017d);
	qca8337_reg_write (0x678, 0x0014017b);
	qca8337_reg_write (0x684, 0x00140177);
	qca8337_reg_write (0x690, 0x0014016f);
	qca8337_reg_write (0x69c, 0x0014015f);
	qca8337_reg_write (0x6a8, 0x0014013f);

	qca8337_reg_write (0x420, 0x00010001);
	qca8337_reg_write (0x448, 0x00010001);
	qca8337_reg_write (0x428, 0x00010001);
	qca8337_reg_write (0x430, 0x00010001);
	qca8337_reg_write (0x440, 0x00010001);
	qca8337_reg_write (0x450, 0x00010001);

	qca8337_reg_write (0x438, 0x00020001);

	qca8337_reg_write (0x424, 0x00002040);
	qca8337_reg_write (0x44c, 0x00002040);
	qca8337_reg_write (0x42c, 0x00001040);
	qca8337_reg_write (0x434, 0x00001040);
	qca8337_reg_write (0x43c, 0x00001040);
	qca8337_reg_write (0x444, 0x00001040);
	qca8337_reg_write (0x454, 0x00001040);

	qca8337_reg_write (0x610, 0x00199d60);
	qca8337_reg_write (0x614, 0x80010002);
	qca8337_reg_write (0x610, 0x0018b7e0);
	qca8337_reg_write (0x614, 0x80020002);

	printf("VLAN Configuration\n");
}

U_BOOT_CMD(
	qca8337_init_one,	1,	0,	do_qca8337_init_one,
	"init qca8337 before booting kernel.",
	"- init qca8337 before booting kernel."
);

int qca8337_init(void)
{
	printf("(QCA8337) ");

	/*
	 * Make sure QCA8337 switches are fully reset. Otherwise, Ethernet may
	 * be unable to be brought up. These pins should be high-active.
	 */
	gpio_set_value(ETH_B_RST_GPIO, 1);
	gpio_set_value(ETH_A_RST_GPIO, 1);
	mdelay(10);
	gpio_set_value(ETH_B_RST_GPIO, 0);
	gpio_set_value(ETH_A_RST_GPIO, 0);
	mdelay(1000);

	qca8337_reg_write (0x88, 0);
	/* ====== MAC 0, RGMII_A ======= */
	/*
	mdio write al_eth1 0x18 0x00 0x0000
	mdio write al_eth1 0x11 0x1e 0x007e
	mdio write al_eth1 0x11 0x1f 0x0000
	*/
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x18, 0x00, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x11, 0x1e, 0x007e);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x11, 0x1f, 0x0000);

	/*
	mdio write al_eth1 0x18 0x00 0x0000
	mdio write al_eth1 0x10 0x02 0x0000
	mdio write al_eth1 0x10 0x03 0x0760
	*/
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x18, 0x00, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x10, 0x02, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x10, 0x03, 0x0760);

	/*
	mdio write al_eth1 0x18 0x00 0x0003
	mdio write al_eth1 0x10 0x12 0x7f7f
	mdio write al_eth1 0x10 0x13 0x007f
	*/
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x18, 0x00, 0x0003);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x10, 0x12, 0x7f7f);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x10, 0x13, 0x007f);

	/*
	mdio write al_eth1 0x18 0x00 0x0000
	mdio write al_eth1 0x13 0x12 0xa545
	mdio write al_eth1 0x13 0x13 0x000e
	*/
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x18, 0x00, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x13, 0x12, 0xa545);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x13, 0x13, 0x000e);

	/*
	mdio write al_eth1 0x18 0x00 0x0000
	mdio write al_eth1 0x10 0x04 0x0000
	mdio write al_eth1 0x10 0x05 0x0760
	*/
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x18, 0x00, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x10, 0x04, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x10, 0x05, 0x0760);

	/* ====== MAC 5, RGMII_B ======= */
	/*
	mdio write al_eth1 0x18 0x00 0x0000
	mdio write al_eth1 0x12 0x08 0x007e
	mdio write al_eth1 0x12 0x09 0x0000
	*/
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x18, 0x00, 0x0000);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x12, 0x08, 0x007e);
	miiphy_write (AL_ETH_RGMII_INTERFACE, 0x12, 0x09, 0x0000);

	printf("Check Status Port 3 status is 0x%x\n", qca8337_reg_read(0x88));
	return 0;
}

