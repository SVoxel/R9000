 /*
   * drivers/gpio/al_gpio.h
   * This file contains the GPIO driver API for the Annapurna Labs
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

#define AL_GPIO_TOTAL_PIN_AMOUNT_MUXED      44
#define AL_GPIO_TOTAL_PIN_AMOUNT            48

/**
 * Initialize the gpio_if structs, using the provided board's address space
 */
int gpio_init(void);

