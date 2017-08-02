 /*
   * arch/arm/include/asm/arch-alpine/al_eth.h
   *
   * Thie file contains Annapurna Labs Ethernet driver declarations
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
#ifndef __AL_ETH_H__
#define __AL_ETH_H__

int al_eth_pci_probe(
	void);

int al_eth_register(
	int		index,
	pci_dev_t	pci_devno,
	uint8_t		rev_id,
	void __iomem	*udma_regs_base,
	void __iomem	*ec_regs_base,
	void __iomem	*mac_regs_base);

int al_eth_board_params_load(
	int index,
	struct al_eth_board_params *params);

int al_eth_board_params_store(
	int index,
	struct al_eth_board_params *params);

int al_eth_mac_link_set(
	int index,
	al_bool sgmii,
	al_bool autoneg,
	uint32_t speed,
	al_bool full_duplex);

int al_eth_serdes_override_set(
	int index,
	al_bool override);

int al_eth_mac_mode_set(
	int index,
	enum al_eth_board_media_type mac_mode);

int al_eth_link_training_enable(
	int index,
	al_bool enable);

int al_eth_link_management_debug_enable(
	int index,
	al_bool enable);

int al_eth_retimer_config_override(
	int index,
	al_bool exist,
	uint8_t i2c_bus_id,
	uint8_t i2c_addr,
	enum al_eth_retimer_channel channel);

#endif

