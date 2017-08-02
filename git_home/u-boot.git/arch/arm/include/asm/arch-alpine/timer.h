 /*
   * arch/arm/include/asm/arch-alpine/timer.h
   * This file contains timer declarations for Annapurna Labs architecture.
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

/* Timer initialization - required to be called after relocation */
int timer_init_r(void);

/* System counter - get low count */
uint32_t sys_counter_cnt_get_low(void);
