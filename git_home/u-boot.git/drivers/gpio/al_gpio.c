 /*
   * drivers/gpio/al_gpio.c
   * This file contains the GPIO driver for the Annapurna Labs
   * architecture.
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
#include <gpio.h>

#include "al_hal_gpio.h"
#include "al_hal_iomap.h"

#define AL_GPIO_ID_TO_DEVICE(id)	((id)/AL_GPIO_DEVICE_PIN_AMOUNT)
#define AL_GPIO_ID_TO_PIN(id)		((id)%AL_GPIO_DEVICE_PIN_AMOUNT)
#define AL_GPIO_DEVICE_NUM		DIV_ROUND_UP( \
					AL_GPIO_TOTAL_PIN_AMOUNT, \
					AL_GPIO_DEVICE_PIN_AMOUNT)

/* Each array element represents a board's GPIO device */
static struct al_gpio_interface gpio_if[AL_GPIO_DEVICE_NUM];

static inline int al_gpio_input_check(unsigned int id, const char *origin)
{
	if (id >= AL_GPIO_TOTAL_PIN_AMOUNT) {
		al_err("%s: Pin id is out of bound. id = %d\n", origin, id);
		return -1;
	}
	return 0;
}

/**
 * Initialize the gpio_if structs, using the provided board's address space
 */
int gpio_init(void)
{
	int i;

	for (i = 0; i < AL_GPIO_NUM; i++)
		al_gpio_init(&gpio_if[i], (void *)(uintptr_t)AL_GPIO_BASE(i));

	return 0;
}

/**
 * Notice that a GPIO should be selected by the MPP (Multi Purpose Pin)
 * multiplexer, before issueing the gpio_request command.
 * Refer to al_hal_muio_mux.h for further details.
 */
int gpio_request(unsigned gpio, const char *label)
{
	int ret = 0;

	ret = al_gpio_input_check(gpio, __func__);
	if (ret != 0)
		return -1;

	/* TODO: add check that the corresponding muio_mux has been selected */

	return 0;
}

int gpio_free(unsigned gpio)
{
	if (al_gpio_input_check(gpio, __func__) != 0)
		return -1;

	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	int dev_idx, pin_idx;
	if (al_gpio_input_check(gpio, __func__) != 0)
		return -1;

	dev_idx = AL_GPIO_ID_TO_DEVICE(gpio);
	pin_idx = AL_GPIO_ID_TO_PIN(gpio);
	al_gpio_dir_set(&gpio_if[dev_idx], pin_idx, AL_GPIO_DIR_IN);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	int dev_idx, pin_idx;

	if (al_gpio_input_check(gpio, __func__) != 0)
		return -1;

	dev_idx = AL_GPIO_ID_TO_DEVICE(gpio);
	pin_idx = AL_GPIO_ID_TO_PIN(gpio);
	al_gpio_dir_set(&gpio_if[dev_idx], pin_idx, AL_GPIO_DIR_OUT);
	al_gpio_set(&gpio_if[dev_idx], pin_idx, value);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	int dev_idx, pin_idx;

	if (al_gpio_input_check(gpio, __func__) != 0)
		return -1;

	dev_idx = AL_GPIO_ID_TO_DEVICE(gpio);
	pin_idx = AL_GPIO_ID_TO_PIN(gpio);

	return al_gpio_get(&gpio_if[dev_idx], pin_idx);
}

int gpio_set_value(unsigned gpio, int value)
{
	int dev_idx, pin_idx;

	if (al_gpio_input_check(gpio, __func__) != 0)
		return -1;

	dev_idx = AL_GPIO_ID_TO_DEVICE(gpio);
	pin_idx = AL_GPIO_ID_TO_PIN(gpio);
	al_gpio_set(&gpio_if[dev_idx], pin_idx, value);

	return 0;
}



