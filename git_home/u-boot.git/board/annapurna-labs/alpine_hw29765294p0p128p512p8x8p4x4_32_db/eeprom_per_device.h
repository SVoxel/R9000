/**
 * board/annapurna-labs/alpine_db/eeprom_per_device.h
 *
 * Annapurna Labs Alpine development board EEPROM per device services
 *
 * Copyright (C) 2014 Annapurna Labs Ltd.
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
#ifndef __EEPROM_PER_DEVICE_H__
#define __EEPROM_PER_DEVICE_H__

#define EEPROM_PER_DEVICE_NUM_MAC_ADDR	3
#define MAC_ADDR_LEN			6

/**
 * Initialize the EEPROM per device information
 *
 * @param	is_valid
 *		An indication whether the information read from the EEPROM
 *		is valid - if not, default information is assumed, and saving
 *		the information is required.
 *
 * @returns	0 upon success
 */
int eeprom_per_device_init(
	int	*is_valid);

/**
 * Get MAC address
 *
 * @param	idx
 *		MAC address index
 *
 * @param	mac_addr
 *		The returned MAC address (MAC_ADDR_LEN bytes)
 *
 * @param	dirty
 *		An indication whether the MAC address is dirty (i.e. not saved
 *		to the EEPROM yet)
 */
void eeprom_per_device_mac_addr_get(
	int	idx,
	uint8_t	*mac_addr,
	int	*dirty);

/**
 * Set MAC address
 *
 * @param	idx
 *		MAC address index
 *
 * @param	mac_addr
 *		The MAC address to set (MAC_ADDR_LEN bytes)
 */
void eeprom_per_device_mac_addr_set(
	int	idx,
	uint8_t	*mac_addr);

/**
 * Save all information to EEPROM
 *
 * @returns	0 upon success
 */
int eeprom_per_device_save(
	void);

#endif
