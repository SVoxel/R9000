 /*
   * drivers/net/al_eth_pci.c
   *
   * Thie file contains Annapurna Labs Ethernet driver PCI handler
   *
   * Copyright (C) 2012 Annapurna Labs Ltd.
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
#include <pci.h>

#include <al_hal_eth.h>
#include "al_hal_iomap.h"
#include "al_eth.h"

#if defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
#include <asm/gpio.h>
#endif

static struct pci_device_id supported[] = {
	{ PCI_VENDOR_ID_ANNAPURNALABS, PCI_DEVICE_ID_AL_ETH },
	{ PCI_VENDOR_ID_ANNAPURNALABS, PCI_DEVICE_ID_AL_ETH_ADVANCED },
	{}
};

int al_eth_pci_probe(
	void)
{
	int err;
	int idx;
	int eth_idx_prev = -1;

	debug("%s()\n", __func__);

	for (idx = 0; ; idx++) {
		int devno;
		uintptr_t iob[6];
		uint16_t word;
		int eth_idx;
		uint8_t rev_id;

		/* Find PCI device(s) */
		devno = pci_find_devices(supported, idx);
		if (devno < 0) {
			debug(
				"%s: pci_find_devices found no more devices\n",
				__func__);
			break;
		}

		debug(
			"%s: pci_find_devices found %d:%d:%d\n",
			__func__,
			PCI_BUS(devno),
			PCI_DEV(devno),
			PCI_FUNC(devno));

		eth_idx = PCI_DEV(devno);

		/* Skip non existing devices while keeping the index */
		for (; (eth_idx_prev + 1) < eth_idx; eth_idx_prev++)
			eth_register(NULL);
		eth_idx_prev = eth_idx;

		if (!((1 << eth_idx) & AL_ETH_ENABLE_VECTOR)) {
			debug(
				"%s: skipping al_eth%d\n",
				__func__,
				eth_idx);
			continue;
		}

		/* Read out device ID and revision ID */
		pci_read_config_byte(devno, PCI_REVISION_ID, &rev_id);

		/* Read out all BARs */
		iob[0] = (uintptr_t)pci_map_bar(devno,
				PCI_BASE_ADDRESS_0, PCI_REGION_MEM);
		iob[2] = (uintptr_t)pci_map_bar(devno,
				PCI_BASE_ADDRESS_2, PCI_REGION_MEM);
		iob[4] = (uintptr_t)pci_map_bar(devno,
				PCI_BASE_ADDRESS_4, PCI_REGION_MEM);

		/* Enable Bus Mastering and memory region */
		pci_write_config_word(devno, PCI_COMMAND,
				PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

		/* Check if mem accesses and Bus Mastering are enabled. */
		pci_read_config_word(devno, PCI_COMMAND, &word);
		if (!(word & PCI_COMMAND_MEMORY) ||
				(!(word & PCI_COMMAND_MASTER))) {
			printf(
				"%s: Failed enabling mem access or bus "
				"mastering!\n",
				__func__);

			debug("%s: PCI command: %04x\n", __func__, word);

			return -EIO;
		}

		err = al_eth_register(
			eth_idx,
			devno,
			rev_id,
			(void __iomem *)iob[AL_ETH_UDMA_BAR],
			(void __iomem *)iob[AL_ETH_EC_BAR],
			(void __iomem *)iob[AL_ETH_MAC_BAR]);
		if (err) {
			printf("%s: al_eth_register failed!\n", __func__);
			return err;
		}
	}
#ifdef CONFIG_QCA8337_SWITCH
extern int qca8337_init(void);
	qca8337_init();
#endif

#if defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
        gpio_set_value(WAN_PHY_RESET_GPIO, 0);
        mdelay(50);
        gpio_set_value(WAN_PHY_RESET_GPIO, 1); // Always pull high
        gpio_set_value(LAN_PHY_RESET_GPIO, 0);
        mdelay(200);
        gpio_set_value(LAN_PHY_RESET_GPIO, 1); // Always pull high
        mdelay(200);
        mdio_gpio_init();
#endif
	return 0;
}

#if defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
#define GPIO_MDC_PIN     0
#define GPIO_MDIO_PIN    1

void Mdc_Pulse(void)  /*Clock line */
{
    /* 1 Mhz */
    udelay(1);
    gpio_set_value(GPIO_MDC_PIN, 0);
    udelay(1);
    gpio_set_value(GPIO_MDC_PIN, 1);
    udelay(1);
}

void Idle(void)
{
    gpio_direction_output(GPIO_MDIO_PIN, 1);
    Mdc_Pulse();
}

void Preamble(void)
{
    char i;

    gpio_direction_output(GPIO_MDIO_PIN, 1);
    /* Transmit Preamble 11....11(32 bits) */
    for(i=0 ; i < 32 ; i++)
        Mdc_Pulse();
    /* Transmit Start of Frame '01' */
    gpio_set_value(GPIO_MDIO_PIN, 0);
    Mdc_Pulse();
    gpio_set_value(GPIO_MDIO_PIN, 1);
    Mdc_Pulse();
}

unsigned int phy_reg_read_by_gpio(char phy_addr,char phy_reg)
{
    char i;
    u16 phy_val;

    Preamble();
    /*OP Code 10*/
    gpio_direction_output(GPIO_MDIO_PIN, 1);
    Mdc_Pulse();
    gpio_set_value(GPIO_MDIO_PIN, 0);
    Mdc_Pulse();
    /*5 bits PHY addr*/
    for(i = 0; i < 5; i++)
    {
        if(phy_addr & 0x10)
        {
            gpio_set_value(GPIO_MDIO_PIN, 1);
        }
        else
        {
            gpio_set_value(GPIO_MDIO_PIN, 0);
        }
        Mdc_Pulse();
        phy_addr <<= 1;
    }
    /*5 bits PHY reg*/
    for(i = 0; i < 5; i++)
    {
        if(phy_reg & 0x10)
        {
            gpio_set_value(GPIO_MDIO_PIN, 1);
        }
        else
        {
            gpio_set_value(GPIO_MDIO_PIN, 0);
        }
        Mdc_Pulse();
        phy_reg <<= 1;
    }
    /*Turnaround Z*/
    gpio_set_value(GPIO_MDIO_PIN, 1);
    Mdc_Pulse();
    gpio_direction_input(GPIO_MDIO_PIN);
    /*Read 16 bits Data*/
    phy_val = 0x0000;
    for ( i = 0; i < 16; i++)
    {
        Mdc_Pulse();
        if (1 == gpio_get_value(GPIO_MDIO_PIN))
            phy_val |= 0x0001;
        if (i < 15)
            phy_val <<= 1;
    }
    Idle();
    gpio_direction_output(GPIO_MDIO_PIN, 1);

    return phy_val;
}

void phy_reg_write_by_gpio(char phy_addr,char phy_reg, unsigned int phy_val)
{
    char i;
    u16 Temp;

    Preamble();
    /*OP Code 01*/
    gpio_direction_output(GPIO_MDIO_PIN, 0);
    Mdc_Pulse();
    gpio_set_value(GPIO_MDIO_PIN, 1);
    Mdc_Pulse();
    /*5 bits PHY addr*/
    for(i = 0; i < 5; i++)
    {
        if(phy_addr & 0x10)
        {
            gpio_set_value(GPIO_MDIO_PIN, 1);
        }
        else
        {
            gpio_set_value(GPIO_MDIO_PIN, 0);
        }
        Mdc_Pulse();
        phy_addr <<= 1;
    }
    /*5 bits PHY reg*/
    for(i = 0; i < 5; i++)
    {
        if(phy_reg & 0x10)
        {
            gpio_set_value(GPIO_MDIO_PIN, 1);
        }
        else
        {
            gpio_set_value(GPIO_MDIO_PIN, 0);
        }
        Mdc_Pulse();
        phy_reg <<= 1;
    }
    /*Turnaround 10*/
    gpio_set_value(GPIO_MDIO_PIN, 1);
    Mdc_Pulse();
    gpio_set_value(GPIO_MDIO_PIN, 0);
    Mdc_Pulse();
    /*Write 16 bits Data*/
    Temp = 0x8000;
    for ( i = 0; i < 16; i++)
    {
        if(phy_val & Temp)
        {
            gpio_set_value(GPIO_MDIO_PIN, 1);
        }
        else
        {
            gpio_set_value(GPIO_MDIO_PIN, 0);
        }
        Mdc_Pulse();
        Temp >>= 1;
    }
    Idle();
}


int mdio_gpio_init(void)
{
    int status;
    u16 phy_addr, phy_reg;
    u16 phy_val;


    /* MDC */
    status = gpio_request(GPIO_MDC_PIN, "gpio_as_mdc");
    if (status < 0)
    {
        printf("request GPIO%d failed(%d)!\n", GPIO_MDC_PIN, status);
        return status;
    }
    gpio_direction_output(GPIO_MDC_PIN, 1);

    /* MDIO */
    status = gpio_request(GPIO_MDIO_PIN, "gpio_as_mdio");
    if (status < 0)
    {
        printf("request GPIO%d failed(%d)!\n", GPIO_MDIO_PIN, status);
        return status;
    }
    gpio_direction_output(GPIO_MDIO_PIN, 0);
    
    phy_reg_write_by_gpio(0x2,0x0, 0x9803);
    phy_val = phy_reg_read_by_gpio(0x2,0x1);
    printf("Check Marvel ID is %x\n", phy_val);
    if (phy_val == 0x1302) {
        printf("QCA8337 is found\n");
    }
    
    phy_reg_write_by_gpio(0x2,0x1, 0x000B);
    /*      0x0009 for 1000base-X
						0x100A for SGMII
						0x000B for 2500base-X   */
    phy_reg_write_by_gpio(0x2,0x0, 0x9520); /* write to port 9 */
    phy_reg_write_by_gpio(0x2,0x0, 0x9540); /* write to port 10 */
    
    phy_reg_write_by_gpio(0x2,0x1, 0x303F); /* 0x203E for 1G,  0x303F for 2.5G */
    /*      force speed/duplex/link , 
						suppose we dont need this.  
						Only for 1G/2.5G fail , debug purpose, set speed according the speed above.   */
    phy_reg_write_by_gpio(0x2,0x0, 0x9521); /* write to port 9 */
    phy_reg_write_by_gpio(0x2,0x0, 0x9541); /* write to port 10 */
    
    /* switch software reset */
    phy_reg_write_by_gpio(0x2,0x1, 0xc001);
    phy_reg_write_by_gpio(0x2,0x0, 0x9764);
    
    phy_reg_write_by_gpio(0x2,0x1, 0x9140);
    phy_reg_write_by_gpio(0x2,0x0, 0x9799);
    phy_reg_write_by_gpio(0x2,0x1, 0x9420);
    phy_reg_write_by_gpio(0x2,0x0, 0x9798);
    
    return 0;
}
#endif
