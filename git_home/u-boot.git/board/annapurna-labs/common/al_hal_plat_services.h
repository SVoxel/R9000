/*******************************************************************************
Copyright (C) 2011 Annapurna Labs Ltd.

This software file is triple licensed: you can use it either under the terms of
Commercial, the GPL, or the BSD license, at your option.

a) If you received this File from Annapurna Labs and you have entered into a
   commercial license agreement (a "Commercial License") with Annapurna Labs,
   the File is licensed to you under the terms of the applicable Commercial
   License.

Alternatively,

b) This file is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
   MA 02110-1301 USA

Alternatively,

c) Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    *	Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *	Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
/**
 * @defgroup group_services Platform Services API
 *  Platform Services API
 *  @{
 * @file   al_plat_services.h
 *
 * @brief  API for Platform services provided for to HAL drivers
 *
 *
 */

#ifndef __PLAT_SERVICES_H__
#define __PLAT_SERVICES_H__

// #include <asm/kernel.h>
#include <asm/io.h>
// #include <linux/printk.h>
#include <common.h>
#include <errno.h>


/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define _al_reg_read8(l)		__raw_readb(l)
#define _al_reg_read16(l)		__raw_readw(l)
#define _al_reg_read32(l)		__raw_readl(l)

#define _al_reg_write8(l,v)		__raw_writeb(v,l)
#define _al_reg_write16(l,v)		__raw_writew(v,l)
#define _al_reg_write32(l,v)		__raw_writel(v,l)
#define _al_reg_write32_relaxed(l,v)	__raw_writel(v,l)

#define AL_PERROR	KERN_ERR
#define AL_PINFO	KERN_INFO
#define AL_PDEBUG	KERN_DEBUG

/**
 * print message
 *
 * @param type of message
 * @param format
 */
#define al_print(...) 		printf(__VA_ARGS__)

/**
 * print error message
 *
 * @param format
 */
#define al_err(...)		printf(__VA_ARGS__)

/**
 * print warning message
 *
 * @param format
 */
#define al_warn(...)		printf(__VA_ARGS__)

/**
 * print info message
 *
 * @param format
 */
#define al_info(...)		debug(__VA_ARGS__)

/**
 * print debug message
 *
 * @param format
 */
#define al_dbg(...)		debug(__VA_ARGS__)

/**
 * Assertion
 *
 * @param condition
 */
#define al_assert(COND)						\
do {								\
	if (!(COND))						\
		al_err(						\
			"%s:%d:%s: Assertion failed! (%s)\n",	\
			__FILE__, __LINE__, __func__, #COND);	\
} while (0)

#ifdef AL_HAL_DEBUG_REG_READ_WRITE
static void al_reg_write8(void *addr, uint8_t val)
{
	al_dbg("al_reg_write8(%p, %02x)\n", addr, val);
	_al_reg_write8(addr, val);
}

static uint8_t al_reg_read8(void *addr)
{
	uint8_t val = _al_reg_read8(addr);
//	al_dbg("al_reg_read8(%p) = %02x\n", addr, val);
	return val;
}

static void al_reg_write16(void *addr, uint16_t val)
{
	al_dbg("al_reg_write16(%p, %04x)\n", addr, val);
	_al_reg_write16(addr, val);
}

static uint16_t al_reg_read16(void *addr)
{
	uint16_t val = _al_reg_read16(addr);
//	al_dbg("al_reg_read16(%p) = %04x\n", addr, val);
	return val;
}

static uint32_t al_reg_read32(void *addr)
{
	uint32_t val = _al_reg_read32(addr);
//	al_dbg("al_reg_read32(%p) = %08x\n", addr, val);
	return val;
}

static void al_reg_write32(void *addr, uint32_t val)
{
	al_dbg("PLD WA %02x %02x %02x %02x %02x %02x %02x %02x\n",
		(uint8_t)((((uint32_t)addr) >> 0) & 0xff),
		(uint8_t)((((uint32_t)addr) >> 8) & 0xff),
		(uint8_t)((((uint32_t)addr) >> 16) & 0xff),
		(uint8_t)((((uint32_t)addr) >> 24) & 0xff),
		(uint8_t)((((uint32_t)val) >> 0) & 0xff),
		(uint8_t)((((uint32_t)val) >> 8) & 0xff),
		(uint8_t)((((uint32_t)val) >> 16) & 0xff),
		(uint8_t)((((uint32_t)val) >> 24) & 0xff));
	_al_reg_write32(addr, val);
}

static void al_reg_write32_relaxed(void *addr, uint32_t val)
{
//	al_dbg("al_reg_write32_relaxed(%p, %08x)\n", addr, val);
	al_reg_write32(addr, val);
}

#else
#define al_reg_read8		_al_reg_read8
#define al_reg_read16		_al_reg_read16
#define al_reg_read32		_al_reg_read32

#define al_reg_write8		_al_reg_write8
#define al_reg_write16		_al_reg_write16
#define al_reg_write32		_al_reg_write32
#define al_reg_write32_relaxed	_al_reg_write32_relaxed
#endif

#ifdef AL_HAL_DEBUG_UDELAY
static void al_udelay(unsigned int delay)
{
	al_dbg("PLD WA 00 00 00 00 %02x %02x %02x %02x\n",
		(uint8_t)((((uint32_t)delay) >> 0) & 0xff),
		(uint8_t)((((uint32_t)delay) >> 8) & 0xff),
		(uint8_t)((((uint32_t)delay) >> 16) & 0xff),
		(uint8_t)((((uint32_t)delay) >> 24) & 0xff));
	udelay(delay);
}
#else
#define al_udelay(v)		udelay(v)
#endif
#define al_msleep(v)		udelay(v * 1000)

#define swap16_to_le(x)		cpu_to_le16(x)
#define swap32_to_le(x)		cpu_to_le32(x)
#define swap64_to_le(x)		cpu_to_le64(x)
#define swap16_from_le(x)	le16_to_cpu(x)
#define swap32_from_le(x)	le32_to_cpu(x)
#define swap64_from_le(x)	le64_to_cpu(x)

#define al_data_memory_barrier()	dmb()

#define al_smp_data_memory_barrier()	

#define al_local_data_memory_barrier()	dmb()

/**
 * Memory set
 *
 * @param memory pointer
 * @param value for setting
 * @param number of bytes to set
 */
#define al_memset(p, val, cnt)  memset(p, val, cnt)

/**
 * memory compare
 *
 * @param  p1 memory pointer
 * @param  p2 memory pointer
 * @param  cnt number of bytes to compare
 *
 * @return 0 if equal, else otherwise
 */
#define al_memcmp(p1, p2, cnt)  memcmp(p1, p2, cnt)

/**
 * memory copy
 *
 * @param  dest memory pointer to destination
 * @param  src memory pointer to source
 * @param  cnt number of bytes to copy
 */
#define al_memcpy(dest, src, cnt)	memcpy(dest, src, cnt)

/**
 * string compare
 *
 * @param  s1 string pointer
 * @param  s2 string pointer
 *
 * @return 0 if equal, else otherwise
 */
#define al_strcmp(s1, s2)		strcmp(s1, s2)

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */
/** @} end of Platform Services API group */
#endif				/* __PLAT_SERVICES_H__ */
