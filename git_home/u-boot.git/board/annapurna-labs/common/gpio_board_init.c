/**
 * board/annapurna-labs/common/gpio_board_init.c
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
#include <common.h>
#include <asm/io.h>

#include <al_hal_muio_mux.h>

#include <al_globals.h>
#include <gpio_board_init.h>

DECLARE_GLOBAL_DATA_PTR;

/* Board GPIO Initialization */
int gpio_board_init(
	const struct gpio_cfg_ent	*cfg,
	int				cnt)
{
	int err;

	err = gpio_init();
	if (err) {
		printf("%s: gpio initialization failed!\n", __func__);
		return err;
	}

	for (; cnt > 0; cnt--, cfg++) {
		if (cfg->gpio_num < AL_GPIO_TOTAL_PIN_AMOUNT_MUXED) {
			err = al_muio_mux_iface_alloc(
				&al_globals.muio_mux,
				AL_MUIO_MUX_IF_GPIO,
				cfg->gpio_num);
			if (err) {
				printf(
					"%s: gpio %d mpp mux allocation failed!\n",
					__func__,
					cfg->gpio_num);
				return err;
			}
		}

		err = gpio_request(cfg->gpio_num, "gpio");
		if (err) {
			printf(
				"%s: gpio %d gpio request failed!\n",
				__func__,
				cfg->gpio_num);
			return err;
		}

		if (cfg->is_output) {
			err = gpio_direction_output(
				cfg->gpio_num, cfg->output_val);
			if (err) {
				printf(
					"%s: gpio %d gpio dir out (%d) failed!\n",
					__func__,
					cfg->gpio_num,
					cfg->output_val);
				return err;
			}
		} else {
			err = gpio_direction_input(cfg->gpio_num);
			if (err) {
				printf(
					"%s: gpio %d gpio dir in failed!\n",
					__func__,
					cfg->gpio_num);
				return err;
			}
		}
	}

	return 0;
}
