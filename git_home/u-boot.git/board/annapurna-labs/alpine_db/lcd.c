 /*
   * board/annapurna-labs/common/lcd.c
   *
   * Thie file contains implementations for the LCD mechanism
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

#include "lcd.h"
#include <common.h>
#include <serial.h>

DECLARE_GLOBAL_DATA_PTR;

#define LCD_NUM_ROWS	2
#define LCD_NUM_COLS	16

/*****************************************************************************/
/*****************************************************************************/
static void lcd_command(uint8_t val)
{
	debug("%s(%02x)\n", __func__, val);

	eserial2_device.putc_raw(0xa0);
	eserial2_device.putc_raw(0x0a);
	eserial2_device.putc_raw(0x08);
	eserial2_device.putc_raw(val);
	eserial2_device.putc_raw(0x0b);
	eserial2_device.putc_raw(0xb0);
}

/*****************************************************************************/
/*****************************************************************************/
static void lcd_data(uint8_t val)
{
	debug("%s(%02x)\n", __func__, val);

	eserial2_device.putc_raw(0xa0);
	eserial2_device.putc_raw(0x0a);
	eserial2_device.putc_raw(0x80);
	eserial2_device.putc_raw(val);
	eserial2_device.putc_raw(0x0b);
	eserial2_device.putc_raw(0xb0);
}

/*****************************************************************************/
/*****************************************************************************/
int lcd_init(void)
{
	unsigned int baudrate_orig = gd->baudrate;

	gd->baudrate = 9600;
	eserial2_device.setbrg();
	gd->baudrate = baudrate_orig;

	lcd_display_clear();
	lcd_command(0x38); /* 8 bit two row 5*7 dot */
	udelay(53);
	lcd_display_control(1, 0, 0);
	lcd_entry_mode_set(1, 0);
	lcd_cursor_set(0, 0);

	return 0;
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_display_clear(void)
{
	lcd_command(0x1);
	udelay(10000);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_return_home(void)
{
	lcd_command(0x2);
	udelay(10000);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_entry_mode_set(int l2r, int scroll)
{
	lcd_command(0x4 | (l2r << 1) | scroll);
	udelay(53);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_display_control(int display_on, int cursor_on, int blink_on)
{
	lcd_command(0x8 | (display_on << 2) | (cursor_on << 1) | blink_on);
	udelay(53);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_cursor_set(int row, int col)
{
	lcd_command(0x80 | (row << 6) | col);
	udelay(53);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_putc(const char c)
{
	lcd_data(c);
	udelay(53);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_puts(const char *s)
{
	for (; *s; s++)
		lcd_putc(*s);
}

/*****************************************************************************/
/*****************************************************************************/
void lcd_printf(const char *fmt, ...)
{
	va_list args;
	static char buf[80];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	lcd_puts(buf);
}

