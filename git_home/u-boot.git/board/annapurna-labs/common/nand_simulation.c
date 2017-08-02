/*******************************************************************************
Copyright (C) 2013 Annapurna Labs Ltd.

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
		notice, this list of conditions and the following disclaimer in
		the documentation and/or other materials provided with the
		distribution.

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
#include "nand_simulation.h"
#include <common.h>
#include "nand.h"

uint8_t nand_id[] = {0x2c, 0xA3, 0x90, 0x26, 0x64}; /* micron MT29f8abbca 8Gb
						       x8 1.8V, page = 4K,
						       spare area = 224B
						       block size = 64 pages */
uint8_t onfi_params_data[] = {0x4F, 0x4E, 0x46, 0x49, 0x02, 0x00, 0x18, 0x00,
	/* 8 - 19 */
	0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 20 - 31 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 32 - 43 */
	0x4D, 0x49, 0x43, 0x52, 0x4F, 0x4E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	/* 44 - 55 */
	0x4D, 0x54, 0x32, 0x39, 0x46, 0x38, 0x47, 0x30, 0x38, 0x41, 0x42, 0x42,
	/* 56 - 67 */
	0x43, 0x41, 0x33, 0x57, 0x20, 0x20, 0x20, 0x20, 0x2C, 0x00, 0x00, 0x00,
	/* 68 - 79 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 80 - 91 */
	0x00, 0x10, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x04, 0x00, 0x00, 0x38, 0x00,
	/* 92 - 103 */
	0x40, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x23, 0x01, 0x50,
	/* 104 - 115 */
	0x00, 0x06, 0x04, 0x01, 0x00, 0x00, 0x04, 0x00, 0x08, 0x01, 0x0E, 0x00,
	/* 116 - 127 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 128 - 139 */
	0x0A, 0x1F, 0x00, 0x1F, 0x00, 0x58, 0x02, 0x10, 0x27, 0x19, 0x00, 0x64,
	/* 140 - 151 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 152 - 163 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 164 - 175 */
	0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x04, 0x80, 0x01, 0x81, 0x04, 0x01,
	/* 176 - 187 */
	0x02, 0x01, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 188 - 199 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 200 - 211 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 212 - 223 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 224 - 235 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 236 - 247 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 248 - 256 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct nand_onfi_params onfi_params;

/* regs */
#define STATUS_REG_READY 0x60
uint8_t status_reg = 0xe0; /* not write protected */

/* mem */
#define NAND_PAGE_SIZE 4096
#define NAND_OOB_SIZE 224
#define NAND_NUM_PAGES 10
struct nand_page {
	uint8_t data[NAND_PAGE_SIZE];
	uint8_t oob[NAND_OOB_SIZE];
};
#define NAND_GLOBAL_SIZE (NAND_NUM_PAGES * (sizeof(struct nand_page)))
struct nand_page global_mem[NAND_NUM_PAGES];

static uint32_t row;
static uint32_t column;
static int byte_count;
static uint8_t *global_mem_ptr;

#define DATA_FIFO_SIZE 8191
uint8_t data_fifo[8192];
uint32_t data_fifo_read;
uint32_t data_fifo_write;

enum nand_simu_state {
	idle,
	/**/
	readid,
	params1,
	read_byte_count1,
	write_byte_count0,
	write_byte_count1,
	/* 5 addr seq  */
	addr0, addr1, addr2, addr3, addr4,
	/* read page */
	read1,
	/* random read */
	rand1,
	/* program page */
	prog1,
	/* erase*/
	erase2,
};
char *state_name[] = {"idle",
	"readid",
	"params1",
	"read_byte_count1",
	"write_byte_count0",
	"write_byte_count1",
	"addr0", "addr1", "addr2", "addr3", "addr4",
	"read1",
	"rand1",
	"prog1",
	"erase2"};

enum nand_simu_state nand_state = idle;
enum nand_simu_state post_row_state;
enum nand_simu_state post_col_state;

static inline void nand_change_state(enum nand_simu_state to)
{
	debug("\t\t\t\tnand state %s --> %s\n",
			state_name[nand_state], state_name[to]);
	nand_state = to;
}

enum addr_cycle {
	FULL_COL_AND_ROW,	/* 5 cycles */
	ROW_ONLY,		/* 3 cycles */
	COL_ONLY		/* 2 cycles */
};

static inline void set_addr_cycle(
		enum addr_cycle cycle,
		enum nand_simu_state post_state)
{
	if (cycle == COL_ONLY) {
		column = 0;
		post_col_state = post_state;
		nand_change_state(addr0);
	} else if (cycle == ROW_ONLY) {
		row = 0;
		post_row_state = post_state;
		nand_change_state(addr2);
	} else {
		column = 0;
		row = 0;
		post_row_state = post_state;
		post_col_state = addr2;
		nand_change_state(addr0);
	}
}

static void read_to_fifo(uint32_t len)
{
	debug("read to fifo (at %d) %d bytes (start: 0x%X)\n", data_fifo_write,
							len, *global_mem_ptr);
	memcpy(&data_fifo[data_fifo_write], global_mem_ptr, len);
	data_fifo_write += len;
	global_mem_ptr += len;
}

int nand_init_simulation(
			void		*obj,
			void __iomem	*regs_base,
			void __iomem	*wrap_regs_base,
			void __iomem	*cmd_buff_base,
			void __iomem	*data_buff_base,
			void		*raid_dma,
			uint32_t	raid_dma_qid,
			char		*name)
{
	int i;
	u16 crc = ONFI_CRC_BASE;
	uint8_t *p;
	int len = 254;

	for (i = 0 ; i < NAND_NUM_PAGES ; i++) {
		memset(global_mem[i].data, 7, NAND_PAGE_SIZE);
		memset(global_mem[i].oob, 0xff, NAND_OOB_SIZE);
	}

	/* set onfi params */
	memset(&onfi_params, 0, sizeof(onfi_params));
	memcpy(&onfi_params, onfi_params_data, sizeof(onfi_params_data));

	p = (uint8_t *)&onfi_params;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	onfi_params.crc = crc;

	debug("======= ONFI ==========\n");
	debug("bytes per page = %d\n", onfi_params.byte_per_page);
	debug("spare bytes per page = %d\n", onfi_params.spare_bytes_per_page);

	return 0;
}

void nand_cmd_ctrl_simulation(void *nand, uint32_t cmd)
{
	static int status_count;
	uint8_t ctrl = (cmd >> 8) & 0xff;
	uint8_t dat = (cmd & 0xff);

	debug("<%s>: dat = 0x%X, ctrl = 0x%X (%X, %X)\n",
			state_name[nand_state], dat, ctrl, ctrl, dat);

	if (ctrl == AL_NAND_COMMAND_TYPE_WAIT_CYCLE_COUNT)
		return;

	switch (nand_state) {
	case idle:
		if (ctrl == AL_NAND_COMMAND_TYPE_ADDRESS) {
			printf(" ERROR: shouldn't get ALE on idle state\n");
			return;
		}

		if (ctrl == AL_NAND_COMMAND_TYPE_STATUS_READ) {
			read_to_fifo(1);
			break;
		} else if (ctrl == AL_NAND_COMMAND_TYPE_DATA_READ_COUNT) {
			byte_count = (dat & 0xff);
			nand_change_state(read_byte_count1);
			break;
		} else if (ctrl == AL_NAND_COMMAND_TYPE_SPARE_READ_COUNT) {
			byte_count = (dat & 0xff);
			nand_change_state(read_byte_count1);
			break;
		}

		switch (dat) {
		case NAND_CMD_READID:
			data_fifo_read = 0;
			data_fifo_write = 0;
			nand_change_state(readid);
			break;

		case NAND_CMD_READ0:
			data_fifo_read = 0;
			data_fifo_write = 0;
			set_addr_cycle(FULL_COL_AND_ROW, read1);
			break;

		case NAND_CMD_STATUS:
			data_fifo_read = 0;
			data_fifo_write = 0;
			/* read status register */
			global_mem_ptr = &status_reg;
			status_count++;
			if (status_count & 1)
				status_reg |= STATUS_REG_READY;
			else
				status_reg &= ~STATUS_REG_READY;
			break;

		case NAND_CMD_SEQIN:
			set_addr_cycle(FULL_COL_AND_ROW, write_byte_count0);
			data_fifo_read = 0;
			data_fifo_write = 0;
			break;

		case NAND_CMD_ERASE1:
			set_addr_cycle(ROW_ONLY, erase2);
			status_reg &= ~STATUS_REG_READY;
			status_count = 0;
			break;

		case NAND_CMD_RNDOUT:
			set_addr_cycle(COL_ONLY, rand1);
			break;

		case NAND_CMD_PARAM:
			nand_change_state(params1);
			break;
		case NAND_CMD_RESET:
			break;
		default:
			debug("cmd not yet simulated! cmd=%d\n", dat);
			break;
		}
		break;

	case readid:
		if (ctrl != AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}

		if (dat == 0x20)
			global_mem_ptr = (uint8_t *)&onfi_params;
		else
			global_mem_ptr = nand_id;
		nand_change_state(idle); /* read id finished */
		break;

	case addr0:
		if (ctrl != AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		column = dat;
		nand_change_state(addr1);
		break;
	case addr1:
		if (ctrl != AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		column |= dat << 8;
		nand_change_state(post_col_state);
		break;
	case addr2:
		if (ctrl != AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		row = dat;
		nand_change_state(addr3);
		break;
	case addr3:
		if (ctrl != AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		row |= dat << 8;
		nand_change_state(addr4);
		break;
	case addr4:
		if (ctrl != AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		row |= (uint64_t)dat << 16;
		nand_change_state(post_row_state);
		/* set as defualt to memory. overide in speciel case */
		global_mem_ptr = ((uint8_t *)&global_mem[row]) + column;
		break;
	case read1:
		if (ctrl == AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting cmd CLE\n", __LINE__);
			return;
		}
		if (dat != NAND_CMD_READSTART) {
			debug("should be 30h command here?\n");
			return;
		}
		debug("Read DONE: row = 0x%X, column = 0x%X\n", row, column);
		nand_change_state(idle);
		break;

	case write_byte_count0:
		if (ctrl  == AL_NAND_COMMAND_TYPE_CMD) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		byte_count = (dat & 0xff);
		nand_change_state(write_byte_count1);
		break;
	case write_byte_count1:
		if (ctrl == AL_NAND_COMMAND_TYPE_CMD) {
			debug("WARNING(%d): expecting ADDRESS cmd\n", __LINE__);
			return;
		}
		byte_count |= ((dat & 0xff) << 8);
		nand_change_state(prog1);
		break;
	case prog1:
		if (ctrl  == AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting cmd CLE\n", __LINE__);
			return;
		}
		if (ctrl == AL_NAND_COMMAND_TYPE_DATA_WRITE_COUNT) {
			byte_count = (dat & 0xff);
			nand_change_state(write_byte_count1);
			break;
		}
		if (dat == NAND_CMD_RNDIN) {
			set_addr_cycle(COL_ONLY, write_byte_count0);
			break;
		}
		if (dat != NAND_CMD_PAGEPROG) {
			debug("WARNING(%d): expecting cmd 10h\n", __LINE__);
			return;
		}

		nand_change_state(idle); /* move to idle but command 70h should
					    be called next to check the program
					    status */
		break;
	case erase2:
		if (ctrl  == AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting CLE cmd\n", __LINE__);
			return;
		}
		if (dat != NAND_CMD_ERASE2) {
			debug("WARNING(%d): expecting cmd D0h\n", __LINE__);
			return;
		}

		memset(&global_mem[row], 0xff, NAND_PAGE_SIZE);
		nand_change_state(idle);
		status_count = 0;
		status_reg &= ~STATUS_REG_READY;
		debug("erase2: status reg = 0x%X\n", status_reg);
		break;
	case rand1:
		if (ctrl == AL_NAND_COMMAND_TYPE_ADDRESS) {
			debug("WARNING(%d): expecting CLE cmd\n", __LINE__);
			return;
		}
		if (dat != NAND_CMD_RNDOUTSTART) {
			debug("WARNING(%d): expecting cmd E0h\n", __LINE__);
			return;
		}
		global_mem_ptr = ((uint8_t *)&global_mem[row]) + column;
		nand_change_state(idle);
		break;
	case params1:
		global_mem_ptr = (uint8_t *)&onfi_params;
		nand_change_state(idle);
		break;
	case read_byte_count1:
		byte_count |= ((dat & 0xff) << 8);
		if (ctrl == AL_NAND_COMMAND_TYPE_DATA_READ_COUNT)
			read_to_fifo(byte_count);
		nand_change_state(idle);
		break;
	default:
		debug("unknown state!\n");
	}
}

int nand_dev_ready_simulation(void *nand)
{
	debug("dev_ready_simulation: return 1\n");

	return 1;
}
int nand_data_buff_read_simulation(
			void		*obj,
			int		num_bytes,
			int		num_bytes_skip_head,
			int		num_bytes_skip_tail,
			uint8_t		*buff)
{
	data_fifo_read += num_bytes_skip_head;

	debug("READ_BUFF! len = %d (from %d)\n", num_bytes, data_fifo_read);

	debug("read start with 0x%X%X%X%X\n", data_fifo[data_fifo_read],
						data_fifo[data_fifo_read + 1],
						data_fifo[data_fifo_read + 2],
						data_fifo[data_fifo_read + 3]);

	memcpy(buff, &data_fifo[data_fifo_read], num_bytes);

	data_fifo_read += num_bytes + num_bytes_skip_tail;

	return 0;
}
int nand_data_buff_write_simulation(
			void		*obj,
			int		num_bytes,
			const uint8_t	*buff)
{
	memcpy(data_fifo, buff, num_bytes);

	memcpy(global_mem_ptr, data_fifo, num_bytes);

	global_mem_ptr += num_bytes;
	data_fifo_read = 0;
	data_fifo_write = 0;

	return 0;
}
int nand_cmd_buff_is_empty_simulation(
			void		*obj)
{
	return 1;
}


int nand_scan_bbt_simulate(struct mtd_info *mtd)
{
	/* this function mark all blockes as good */
	struct nand_chip *this = mtd->priv;
	int i;

	debug("scan BBT simulation!\n");

	for (i = 0 ; i < 500 ; i++)
		this->bbt[i] = 1;

	return 0;
}

int nand_properties_decode_simulate(
	struct al_pbs_regs			*regs_base,
	struct al_nand_dev_properties		*dev_properties,
	struct al_nand_ecc_config		*ecc_config,
	struct al_nand_extra_dev_properties	*dev_ext_props)
{
	dev_ext_props->eccIsEnabled = 1;
	dev_ext_props->pageSize = NAND_PAGE_SIZE;
	ecc_config->spareAreaOffset = 98 + dev_ext_props->pageSize;
	ecc_config->messageSize = 1;
	dev_properties->num_col_cyc = 3;
	dev_properties->num_row_cyc = 2;


	return 0;
}

