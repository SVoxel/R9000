 /*
   * board/annapurna-labs/common/lcd.h
   *
   * Thie file contains an API declarations for the LCD
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

#ifndef __LCD_H__
#define __LCD_H__

/**
 * LCD initialization
 *
 * @returns	0 upon success
 */
int lcd_init(void);

/**
 * LCD clear display
 */
void lcd_display_clear(void);

/**
 * LCD return cursor to home position (top, left)
 */
void lcd_return_home(void);

/**
 * LCD set entry mode
 *
 * @param	l2r - left to right, or right to left
 * @param	scroll - scroll on, or off
 */
void lcd_entry_mode_set(int l2r, int scroll);

/**
 * LCD display control
 *
 * @param	display_on - enable display
 * @param	cursor_on - enable cursor
 * @param	blink_on - enable cursor blink
 */
void lcd_display_control(int display_on, int cursor_on, int blink_on);

/**
 * LCD set cursor position
 *
 * @param	row - row (0 - 1)
 * @param	col - column (0 - 15)
 */
void lcd_cursor_set(int row, int col);

/**
 * LCD single character printing
 *
 * @param	c - character to display on the LCD
 */
void lcd_putc(const char c);

/**
 * LCD string printing
 *
 * @param	s - string to display on the LCD
 */
void lcd_puts(const char *s);

/**
 * LCD formatted string printing
 *
 * @param	fmt - formatted string to display on the LCD
 * @param	...
 */
void lcd_printf(const char *fmt, ...);

#endif

