/**
 * board/annapurna-labs/alpine_db/eeprom_per_device.c
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

#include <common.h>
#include <i2c.h>
#include <al_globals.h>
#include <nand.h>

#include "eeprom_per_device.h"
#include "eeprom_per_device_layout.h"

#define I2C_ADDR_LEN		2

static struct {
	int	mac_addr_dirty[EEPROM_PER_DEVICE_NUM_MAC_ADDR];
	uint8_t	mac_addr[EEPROM_PER_DEVICE_NUM_MAC_ADDR][MAC_ADDR_LEN];
} eeprom_per_device;

/*
 * Gets the ethernet address from the ART partition table and return the value
 */
int get_eth_mac_address(u_char *enetaddr, unsigned int no_of_macs)
{
	s32 ret = 0 ;
	size_t length = (MAC_ADDR_LEN * no_of_macs);
	loff_t art_offset;
	u_char buf;

	/*
	 * ART partition 0th position (6 * 4) 24 bytes will contain the
	 * 4 MAC Address. First 0-5 bytes for GMAC0, Second 6-11 bytes
	 * for GMAC1, 12-17 bytes for GMAC2 and 18-23 bytes for GMAC3
	 */
	art_offset = BOARDCAL;
	ret = nand_read_skip_bad(&nand_info[0], art_offset, &length, NULL, 0x20000, enetaddr);
	if (ret < 0)
		printf("%s: ART partition read failed..\n", __func__);

	buf = enetaddr[0];
	enetaddr[0] = enetaddr[6];
	enetaddr[6] = buf;
	buf = enetaddr[1];
	enetaddr[1] = enetaddr[7];
	enetaddr[7] = buf;
	buf = enetaddr[2];
	enetaddr[2] = enetaddr[8];
	enetaddr[8] = buf;
	buf = enetaddr[3];
	enetaddr[3] = enetaddr[9];
	enetaddr[9] = buf;
	buf = enetaddr[4];
	enetaddr[4] = enetaddr[10];
	enetaddr[10] = buf;
	buf = enetaddr[5];
	enetaddr[5] = enetaddr[11];
	enetaddr[11] = buf;
	return ret;
}

/******************************************************************************
 ******************************************************************************/
int eeprom_per_device_init(
	int	*is_valid)
{
	uint8_t buff[EEPROM_PER_DEVICE_LAYOUT_SIZE];
	int err;
	int i;

	err = i2c_read(al_globals.bootstraps.i2c_preload_addr,
		EEPROM_PER_DEVICE_LAYOUT_OFFSET, I2C_ADDR_LEN, buff,
		EEPROM_PER_DEVICE_LAYOUT_SIZE);
	if (err)
		printf("%s: i2c read failed!\n", __func__);

	if ((!err) && (buff[EEPROM_PER_DEVICE_LAYOUT_MAGIC_NUM_OFFSET] !=
				EEPROM_PER_DEVICE_LAYOUT_MAGIC_NUM)) {
		printf("%s: no valid information found!\n", __func__);
		err = -EINVAL;
	}

	if (err) {
		u_char enet_addr[EEPROM_PER_DEVICE_NUM_MAC_ADDR * MAC_ADDR_LEN];
		int ret;

		printf("%s#%d: Try reading from NAND flash\n", __func__, __LINE__);
		memset(enet_addr, 0, sizeof(enet_addr));
		ret = get_eth_mac_address(enet_addr, EEPROM_PER_DEVICE_NUM_MAC_ADDR);

		if (ret < 0) {
			*is_valid = 0;
			for (i = 0; i < EEPROM_PER_DEVICE_NUM_MAC_ADDR; i++) {
				memset(eeprom_per_device.mac_addr[i], 0xff, MAC_ADDR_LEN);
				eeprom_per_device.mac_addr_dirty[i] = 1;
			}
		}
		else {
			printf("%s#%d: Reading from NAND flash OK\n", __func__, __LINE__);
			*is_valid = 1;
			for (i = 0; i < EEPROM_PER_DEVICE_NUM_MAC_ADDR; i++) {
				memcpy(eeprom_per_device.mac_addr[i],
					&enet_addr[i * MAC_ADDR_LEN],
					MAC_ADDR_LEN);
				eeprom_per_device.mac_addr_dirty[i] = 0;
			}
		}

		return 0;
	}

	for (i = 0; i < EEPROM_PER_DEVICE_NUM_MAC_ADDR; i++) {
		memcpy(eeprom_per_device.mac_addr[i],
			&buff[EEPROM_PER_DEVICE_LAYOUT_MAC_ADDRESS_OFFSET(i)],
			MAC_ADDR_LEN);
		eeprom_per_device.mac_addr_dirty[i] = 0;
	}

	*is_valid = 1;

	return 0;
}

/******************************************************************************
 ******************************************************************************/
void eeprom_per_device_mac_addr_get(
	int	idx,
	uint8_t	*mac_addr,
	int	*dirty)
{
	memcpy(mac_addr, eeprom_per_device.mac_addr[idx], MAC_ADDR_LEN);
	*dirty = eeprom_per_device.mac_addr_dirty[idx];
}

/******************************************************************************
 ******************************************************************************/
void eeprom_per_device_mac_addr_set(
	int	idx,
	uint8_t	*mac_addr)
{
	memcpy(eeprom_per_device.mac_addr[idx], mac_addr, MAC_ADDR_LEN);
	eeprom_per_device.mac_addr_dirty[idx] = 1;
}

/******************************************************************************
 ******************************************************************************/
int _eeprom_per_device_mac_addr_save(
	int idx)
{
	int err = 0;
	int i;

	for (i = 0; (i < MAC_ADDR_LEN) && (!err); i++) {
		err = i2c_write(al_globals.bootstraps.i2c_preload_addr,
			EEPROM_PER_DEVICE_LAYOUT_OFFSET +
			EEPROM_PER_DEVICE_LAYOUT_MAC_ADDRESS_OFFSET(idx) + i,
			I2C_ADDR_LEN,
			&eeprom_per_device.mac_addr[idx][i], 1);
		udelay(11000);
	}
	if (!err)
		eeprom_per_device.mac_addr_dirty[idx] = 0;

	return err;
}

/******************************************************************************
 ******************************************************************************/
int eeprom_per_device_save(
	void)
{
	int err = 0;
	int i;

	for (i = 0; i < EEPROM_PER_DEVICE_NUM_MAC_ADDR; i++) {
		if (_eeprom_per_device_mac_addr_save(i))
			err = -EIO;
	}

	if (!err) {
		uint8_t mn = EEPROM_PER_DEVICE_LAYOUT_MAGIC_NUM;

		err = i2c_write(al_globals.bootstraps.i2c_preload_addr,
			EEPROM_PER_DEVICE_LAYOUT_OFFSET +
			EEPROM_PER_DEVICE_LAYOUT_MAGIC_NUM_OFFSET,
			I2C_ADDR_LEN, &mn, 1);
		udelay(11000);
	}

	if (err)
		printf("%s: save failed!\n", __func__);

	return err;
}

