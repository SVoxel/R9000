/**
 * board/annapurna-labs/common/gpio_board_init.h
 *
 * Board GPIO initialization service
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

#include <asm/gpio.h>

/* Board GPIO configuration entry */
struct gpio_cfg_ent {
	/* GPIO number */
	int	gpio_num;

	/* Configure GPIO as output */
	int	is_output;

	/* Initial output value for output */
	int	output_val;
};

/**
 * Board GPIO Initialization
 *
 * @param[in]  cfg
 *             Board GPIO configuration entry array
 *
 * @param[in]  cnt
 *             Number of board GPIO configuration entries
 *
 * @return 0 upon success
 */
int gpio_board_init(
	const struct gpio_cfg_ent	*cfg,
	int				cnt);
