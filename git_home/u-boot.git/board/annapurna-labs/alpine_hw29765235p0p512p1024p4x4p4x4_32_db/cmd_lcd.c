 /*
   * board/annapurna-labs/alpine_db/cmd_lcd.c
   *
   * Thie file contains a U-Boot command for LCD operations
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
#include <command.h>

#include "lcd.h"

int do_lcd_print(
	cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	lcd_display_clear();

	if (argc > 1) {
		lcd_cursor_set(0, 0);
		lcd_puts(argv[1]);
	}

	if (argc > 2) {
		lcd_cursor_set(1, 0);
		lcd_puts(argv[2]);
	}

	return 0;
}


U_BOOT_CMD(
	lcd_print, 3, 0, do_lcd_print,
	"Clears LCD display",
	"<line 1> <line 2>\n");

