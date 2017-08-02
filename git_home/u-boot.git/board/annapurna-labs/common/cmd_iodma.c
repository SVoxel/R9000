 /*
   * board/annapurna-labs/common/cmd_iodma.c
   *
   * Thie file contains a U-Boot commands for using the RAID and memory services
   * accelerator.
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
#include <pci.h>
#include <malloc.h>
#include <iodma.h>

int do_iodma_memcpy(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	const char *dst_char;
	const char *src_char;
	const char *size_char;
	al_phys_addr_t dst;
	al_phys_addr_t src;
	uint64_t size;
	int err;

	if (argc < 4) {
		printf("Syntax error!\n\n");
		return -1;
	}

	dst_char = argv[1];
	src_char = argv[2];
	size_char = argv[3];

	dst = simple_strtoull(dst_char, NULL, 16);
	src = simple_strtoull(src_char, NULL, 16);
	size = simple_strtoull(size_char, NULL, 16);

	err = iodma_memcpy(dst, src, size);
	if (err) {
		printf("iodma_memcpy failed!\n");
		return -1;
	}

	return 0;
}

int do_iodma_memset(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	const char *dst_char;
	const char *val_char;
	const char *size_char;
	al_phys_addr_t dst;
	uint8_t val;
	uint64_t size;
	int err;

	if (argc < 4) {
		printf("Syntax error!\n\n");
		return -1;
	}

	dst_char = argv[1];
	val_char = argv[2];
	size_char = argv[3];

	dst = simple_strtoull(dst_char, NULL, 16);
	val = simple_strtoul(val_char, NULL, 16);
	size = simple_strtoull(size_char, NULL, 16);

	err = iodma_memset(dst, val, size);
	if (err) {
		printf("iodma_memset failed!\n");
		return -1;
	}

	return 0;
}

int do_iodma_init(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err;

	err = iodma_init();
	if (err) {
		printf("iodma failed");
		return -1;
	}

	return 0;
}

int do_iodma_terminate(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	iodma_terminate();

	return 0;
}

U_BOOT_CMD(
	iodma_memcpy, 4, 1, do_iodma_memcpy,
	"Memory copy using the RAID and memory services accelerator",
	"Before the first use, iodma_init must be called.\n "
	"iodma_memcpy args:\n\n"
	" - dest - pointer to destination address (in hex)\n"
	" - src - pointer to source address (in hex)\n"
	" - size - size to copy (in hex)\n");

U_BOOT_CMD(
	iodma_memset, 4, 1, do_iodma_memset,
	"Memory set using the RAID and memory services accelerator",
	"Before the first use, iodma_init must be called.\n "
	"iodma_memset args:\n\n"
	" - dest - pointer to destination address (in hex)\n"
	" - val - value to be set (8 bits hex)\n"
	" - size - size to copy (in hex)\n");

U_BOOT_CMD(
	iodma_init, 4, 1, do_iodma_init,
	"Initialize the RAID and memory services accelerator",
	"This command must be called before using iodma_memcpy\n");

U_BOOT_CMD(
	iodma_terminate, 4, 1, do_iodma_terminate,
	"Terminates the RAID and memory services accelerator",
	"\n");
