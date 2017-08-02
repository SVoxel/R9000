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
#include <common.h>
#include <malloc.h>
#include <libfdt_env.h>
#include <libfdt.h>
#include "nand.h"
#include "al_hal_reg_utils.h"
#include "al_hal_pbs_regs.h"
#include "al_hal_iomap.h"

#ifndef CONFIG_AL_NAND_SIMULATE
#include "al_hal_nand.h"
#else
#include "nand_simulation.h"
#endif

#define WAIT_EMPTY_CMD_FIFO_TIME_OUT		1000000
#define NAND_CMD_SET_FEATURES			0xef
#define AL_NAND_MAX_ONFI_TIMING_MODE		al_nand_max_onfi_timing_mode_get()

static int al_nand_max_onfi_timing_mode = -1;

static struct nand_chip nand_chip[CONFIG_SYS_MAX_NAND_DEVICE];

struct nand_data {
	struct al_nand_ctrl_obj nand_obj;
	uint8_t word_cache[4];
	int cache_pos;
	struct nand_ecclayout nand_oob;
	uint32_t cw_size;
	struct al_nand_ecc_config ecc_config;
};
/*
 * Addressing RMN: 2903
 *
 * RMN description:
 * NAND timing parameters that are used in the non-manual mode are wrong and
 * reduce performance.
 * Replacing with the manual parameters to increase speed
 */
#define AL_NAND_NSEC_PER_CLK_CYCLES 2.666
#define NAND_CLK_CYCLES(nsec) ((nsec) / (AL_NAND_NSEC_PER_CLK_CYCLES))

const struct al_nand_device_timing al_nand_manual_timing[] = {
	{
		.tSETUP = NAND_CLK_CYCLES(14),
		.tHOLD = NAND_CLK_CYCLES(22),
		.tWRP = NAND_CLK_CYCLES(54),
		.tRR = NAND_CLK_CYCLES(43),
		.tWB = NAND_CLK_CYCLES(206),
		.tWH = NAND_CLK_CYCLES(32),
		.tINTCMD = NAND_CLK_CYCLES(86),
		.readDelay = NAND_CLK_CYCLES(3)
	},
	{
		.tSETUP = NAND_CLK_CYCLES(14),
		.tHOLD = NAND_CLK_CYCLES(14),
		.tWRP = NAND_CLK_CYCLES(27),
		.tRR = NAND_CLK_CYCLES(22),
		.tWB = NAND_CLK_CYCLES(104),
		.tWH = NAND_CLK_CYCLES(19),
		.tINTCMD = NAND_CLK_CYCLES(43),
		.readDelay = NAND_CLK_CYCLES(3)
	},
	{
		.tSETUP = NAND_CLK_CYCLES(14),
		.tHOLD = NAND_CLK_CYCLES(14),
		.tWRP = NAND_CLK_CYCLES(19),
		.tRR = NAND_CLK_CYCLES(22),
		.tWB = NAND_CLK_CYCLES(104),
		.tWH = NAND_CLK_CYCLES(16),
		.tINTCMD = NAND_CLK_CYCLES(43),
		.readDelay = NAND_CLK_CYCLES(3)
	},
	{
		.tSETUP = NAND_CLK_CYCLES(14),
		.tHOLD = NAND_CLK_CYCLES(14),
		.tWRP = NAND_CLK_CYCLES(16),
		.tRR = NAND_CLK_CYCLES(22),
		.tWB = NAND_CLK_CYCLES(104),
		.tWH = NAND_CLK_CYCLES(11),
		.tINTCMD = NAND_CLK_CYCLES(27),
		.readDelay = NAND_CLK_CYCLES(3)
	},
	{
		.tSETUP = NAND_CLK_CYCLES(14),
		.tHOLD = NAND_CLK_CYCLES(14),
		.tWRP = NAND_CLK_CYCLES(14),
		.tRR = NAND_CLK_CYCLES(22),
		.tWB = NAND_CLK_CYCLES(104),
		.tWH = NAND_CLK_CYCLES(11),
		.tINTCMD = NAND_CLK_CYCLES(27),
		.readDelay = NAND_CLK_CYCLES(3)
	},
	{
		.tSETUP = NAND_CLK_CYCLES(14),
		.tHOLD = NAND_CLK_CYCLES(14),
		.tWRP = NAND_CLK_CYCLES(14),
		.tRR = NAND_CLK_CYCLES(22),
		.tWB = NAND_CLK_CYCLES(104),
		.tWH = NAND_CLK_CYCLES(11),
		.tINTCMD = NAND_CLK_CYCLES(27),
		.readDelay = NAND_CLK_CYCLES(3)
	}
};

static inline struct nand_data *nand_data_get(
				struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;

	return nand_chip->priv;
}

static void nand_cw_size_get(
			int		num_bytes,
			uint32_t	*cw_size,
			uint32_t	*cw_count)
{
	num_bytes = AL_ALIGN_UP(num_bytes, 4);

	if (num_bytes < *cw_size)
		*cw_size = num_bytes;

	if (0 != (num_bytes % *cw_size))
		*cw_size = num_bytes / 4;

	BUG_ON(num_bytes % *cw_size);

	*cw_count = num_bytes / *cw_size;
}

static void nand_send_byte_count_command(
			struct al_nand_ctrl_obj		*nand_obj,
			enum al_nand_command_type	cmd_id,
			uint16_t			len)
{
	uint32_t cmd;

	cmd = AL_NAND_CMD_SEQ_ENTRY(
			cmd_id,
			(len & 0xff));

	al_nand_cmd_single_execute(nand_obj, cmd);

	cmd = AL_NAND_CMD_SEQ_ENTRY(
			cmd_id,
			((len & 0xff00) >> 8));

	al_nand_cmd_single_execute(nand_obj, cmd);
}

static void nand_wait_cmd_fifo_empty(
			struct al_nand_ctrl_obj		*nand_obj)
{
	int cmd_buff_empty;
	uint32_t i = WAIT_EMPTY_CMD_FIFO_TIME_OUT;

	while (i > 0) {
		cmd_buff_empty = al_nand_cmd_buff_is_empty(nand_obj);
		if (cmd_buff_empty)
			break;

		udelay(1);
		i--;
	}

	if (i == 0)
		printf("Wait for empty cmd fifo for more than a sec!\n");
}

void nand_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
	uint32_t cmd;
	enum al_nand_command_type type;
	struct nand_data *nand;

	if ((ctrl & (NAND_CLE | NAND_ALE)) == 0)
		return;

	nand = nand_data_get(mtd);
	nand->cache_pos = -1;

	type = ((ctrl & NAND_CTRL_CLE) == NAND_CTRL_CLE) ?
					AL_NAND_COMMAND_TYPE_CMD :
					AL_NAND_COMMAND_TYPE_ADDRESS;

	cmd = AL_NAND_CMD_SEQ_ENTRY(type, (dat & 0xff));
	debug("nand_cmd_ctrl: dat=0x%x, ctrl=0x%x, type=0x%x, cmd=0x%x\n",
						dat, ctrl, type, cmd);

	al_nand_cmd_single_execute(&nand->nand_obj, cmd);

	nand_wait_cmd_fifo_empty(&nand->nand_obj);

	if ((dat == NAND_CMD_PAGEPROG) && (ctrl == NAND_CTRL_CLE)) {
		cmd = AL_NAND_CMD_SEQ_ENTRY(
				AL_NAND_COMMAND_TYPE_WAIT_FOR_READY,
				0);

		al_nand_cmd_single_execute(&nand->nand_obj, cmd);

		nand_wait_cmd_fifo_empty(&nand->nand_obj);

		al_nand_wp_set_enable(&nand->nand_obj, 1);
		al_nand_tx_set_enable(&nand->nand_obj, 0);
	}
}

void nand_dev_select(struct mtd_info *mtd, int chipnr)
{
	struct nand_data *nand;

	if (chipnr < 0)
		return;

	nand = nand_data_get(mtd);
	al_nand_dev_select(&nand->nand_obj, chipnr);

	debug("nand_dev_select: chipnr = %d\n", chipnr);
}

int nand_dev_ready(struct mtd_info *mtd)
{
	int is_ready = 0;
	struct nand_data *nand;

	nand = nand_data_get(mtd);
	is_ready = al_nand_dev_is_ready(&nand->nand_obj);

	debug("nand_dev_ready: ready = %d\n", is_ready);

	return is_ready;
}

/*
 * read len bytes from the nand device.
 */
void nand_read_buff(struct mtd_info *mtd, uint8_t *buf, int len)
{
	uint32_t cw_size;
	uint32_t cw_count;
	struct nand_data *nand;
	void __iomem *data_buff;

	debug("nand_read_buff: read len = %d\n", len);

	nand = nand_data_get(mtd);
	cw_size = nand->cw_size;

	BUG_ON(len & 3);
	BUG_ON(nand->cache_pos != -1);

	nand_cw_size_get(len, &cw_size, &cw_count);

	al_nand_cw_config(
			&nand->nand_obj,
			cw_size,
			cw_count);

	while (cw_count--)
		nand_send_byte_count_command(&nand->nand_obj,
				AL_NAND_COMMAND_TYPE_DATA_READ_COUNT,
				cw_size);

	data_buff = al_nand_data_buff_base_get(&nand->nand_obj);
	memcpy(buf, data_buff, len);
}

/*
 * read byte from the device.
 * read byte is not supported by the controller so this function reads
 * 4 bytes as a cache and use it in the next calls.
 */
uint8_t nand_read_byte_from_fifo(struct mtd_info *mtd)
{
	uint8_t ret_val;
	struct nand_data *nand;

	nand = nand_data_get(mtd);

	if (nand->cache_pos == -1) {
		nand_read_buff(mtd, nand->word_cache, 4);
		nand->cache_pos = 0;
	}

	ret_val = nand->word_cache[nand->cache_pos];
	nand->cache_pos++;
	if (nand->cache_pos == 4)
		nand->cache_pos = -1;

	return ret_val;
}

u16 nand_read_word(struct mtd_info *mtd)
{
	/* shouldn't be called */
	BUG();

	return 0;
}
/*
 * writing buffer to the nand device.
 * this func will wait for the write to be complete
 */
void nand_write_buff(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	uint32_t cw_size;
	uint32_t cw_count;
	struct nand_data *nand;
	void __iomem *data_buff;

	debug("nand_write_buff: len = %d\n", len);

	nand = nand_data_get(mtd);
	cw_size = nand->cw_size;

	al_nand_tx_set_enable(&nand->nand_obj, 1);
	al_nand_wp_set_enable(&nand->nand_obj, 0);

	nand_cw_size_get(len, &cw_size, &cw_count);

	al_nand_cw_config(
			&nand->nand_obj,
			cw_size,
			cw_count);
	while (cw_count--)
		nand_send_byte_count_command(&nand->nand_obj,
				AL_NAND_COMMAND_TYPE_DATA_WRITE_COUNT,
				cw_size);


	data_buff = al_nand_data_buff_base_get(&nand->nand_obj);
	memcpy(data_buff, buf, len);

	/* enable wp and disable tx will be executed after commands
	 * NAND_CMD_PAGEPROG and AL_NAND_COMMAND_TYPE_WAIT_FOR_READY will be
	 * sent to make sure all data were written.
	 */
}

int nand_init_size(struct mtd_info *mtd, struct nand_chip *this, u8 *id_data)
{
	printf("ERROR! init size shouldn't be called on onfi device\n");

	return -1;
}

/*****************/
/* ecc functions */
/*****************/
#ifdef CONFIG_AL_NAND_ECC_SUPPORT
static inline int is_empty_block(uint8_t *buf, int len)
{
	int i;

	for (i = 0 ; i < len ; i++)
		if (buf[i] != 0xff)
			return 0;

	return 1;
}
/*
 * read page with HW ecc support (corrected and uncorrected stat will be
 * updated).
 */
int ecc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
			uint8_t *buf, int oob_required, int page)
{
	int bytes = chip->ecc.layout->eccbytes;
	struct nand_data *nand;

	debug("ecc_read_page: read page %d\n", page);

	BUG_ON(oob_required);

	nand = nand_data_get(mtd);

	/* Clear TX/RX ECC state machine */
	al_nand_tx_set_enable(&nand->nand_obj, 1);
	al_nand_tx_set_enable(&nand->nand_obj, 0);

	al_nand_uncorr_err_clear(&nand->nand_obj);
	al_nand_corr_err_clear(&nand->nand_obj);

	al_nand_ecc_set_enabled(&nand->nand_obj, 1);

	chip->cmdfunc(mtd, NAND_CMD_READOOB,
			chip->ecc.layout->eccpos[0], page);

	nand_send_byte_count_command(&nand->nand_obj,
				AL_NAND_COMMAND_TYPE_SPARE_READ_COUNT,
				bytes);

	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0x00, -1);

	chip->read_buf(mtd, buf, mtd->writesize);

	if (0 != al_nand_uncorr_err_get(&nand->nand_obj)) {
		/* the ECC in BCH algorithm will find an uncorrected errors
		 * while trying to read an empty page.
		 * to avoid error messeges and failures in the upper layer,
		 * don't update the statistics in this case */
		if ((nand->ecc_config.algorithm != AL_NAND_ECC_ALGORITHM_BCH)
			|| (!is_empty_block(buf, mtd->writesize))) {
			mtd->ecc_stats.failed++;
			printf("uncorrected errors found in page %d! (increased to %d)\n",
				page, mtd->ecc_stats.failed);
		}
	}

	if (0 != al_nand_corr_err_get(&nand->nand_obj)) {
		mtd->ecc_stats.corrected++;
		printf("ecc_read_page: corrected increased to %d\n",
						mtd->ecc_stats.corrected);
	}

	debug("ecc_read_page: corrected = %d\n", mtd->ecc_stats.corrected);

	al_nand_ecc_set_enabled(&nand->nand_obj, 0);

	return 0;
}

int ecc_read_subpage(struct mtd_info *mtd, struct nand_chip *chip,
			uint32_t offs, uint32_t len, uint8_t *buf, int page)
{
	printf("ERROR: read subpage not supported!\n");
	return -1;
}
/*
 * program page with HW ecc support.
 */
int ecc_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			const uint8_t *buf, int oob_required)
{
	int bytes = chip->ecc.layout->eccbytes;
	uint32_t cmd;
	struct nand_data *nand;

	debug("ecc_write_page\n");

	BUG_ON(oob_required);

	nand = nand_data_get(mtd);

	al_nand_ecc_set_enabled(&nand->nand_obj, 1);

	nand_write_buff(mtd, buf, mtd->writesize);

	chip->cmdfunc(mtd, NAND_CMD_RNDIN,
			mtd->writesize + chip->ecc.layout->eccpos[0], -1);

	cmd = AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_WAIT_CYCLE_COUNT,
			0);

	al_nand_tx_set_enable(&nand->nand_obj, 1);
	al_nand_wp_set_enable(&nand->nand_obj, 0);

	al_nand_cmd_single_execute(&nand->nand_obj, cmd);

	nand_send_byte_count_command(&nand->nand_obj,
				AL_NAND_COMMAND_TYPE_SPARE_WRITE_COUNT,
				bytes);

	nand_wait_cmd_fifo_empty(&nand->nand_obj);

	al_nand_wp_set_enable(&nand->nand_obj, 1);
	al_nand_tx_set_enable(&nand->nand_obj, 0);

	al_nand_ecc_set_enabled(&nand->nand_obj, 0);

	return 0;
}
#endif

static enum al_nand_ecc_bch_num_corr_bits bch_num_bits_convert(
							unsigned int bits)
{
	switch (bits) {
	case 4:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_4;
	case 8:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_8;
	case 12:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_12;
	case 16:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_16;
	case 20:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_20;
	case 24:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_24;
	case 28:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_28;
	case 32:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_32;
	case 36:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_36;
	case 40:
		return AL_NAND_ECC_BCH_NUM_CORR_BITS_40;
	default:
		BUG();
	}

	return AL_NAND_ECC_BCH_NUM_CORR_BITS_8;
}

static enum al_nand_device_page_size page_size_bytes_convert(
							unsigned int bytes)
{
	switch (bytes) {
	case 2048:
		return AL_NAND_DEVICE_PAGE_SIZE_2K;
	case 4096:
		return AL_NAND_DEVICE_PAGE_SIZE_4K;
	case 8192:
		return AL_NAND_DEVICE_PAGE_SIZE_8K;
	case 16384:
		return AL_NAND_DEVICE_PAGE_SIZE_16K;
	default:
		BUG();
	}

	return AL_NAND_DEVICE_PAGE_SIZE_4K;
}

static void nand_set_timing_mode(
			struct nand_data *nand,
			enum al_nand_device_timing_mode timing)
{

	uint32_t cmds[] = {
		AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_CMD, NAND_CMD_SET_FEATURES),
		AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_ADDRESS, 0xfa),
		AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_STATUS_WRITE, timing),
		AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_STATUS_WRITE, 0x00),
		AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_STATUS_WRITE, 0x00),
		AL_NAND_CMD_SEQ_ENTRY(
			AL_NAND_COMMAND_TYPE_STATUS_WRITE, 0x00)};

	al_nand_cmd_seq_execute(&nand->nand_obj, cmds, ARRAY_SIZE(cmds));

	nand_wait_cmd_fifo_empty(&nand->nand_obj);
}

static unsigned int al_nand_max_onfi_timing_mode_get(void)
{
	if (al_nand_max_onfi_timing_mode < 0) {
		printf("%s: called too early!\n", __func__);
		return AL_NAND_MAX_ONFI_TIMING_MODE_DEFAULT;
	}

	return (unsigned int)al_nand_max_onfi_timing_mode;
}

static void nand_onfi_config_set(
		struct nand_chip *nand,
		struct al_nand_dev_properties *device_properties,
		struct al_nand_ecc_config *ecc_config)
{
	enum al_nand_device_page_size onfi_page_size;
	int i;
	uint16_t async_timing_mode;

	/* since async_timing_mode is not 16bit align in the struct
	 * onfi_params it can't be used directly */
	memcpy(&async_timing_mode, &nand->onfi_params.async_timing_mode,
				sizeof(nand->onfi_params.async_timing_mode));

	/* find the max timinig mode supported by the device and below
	 * AL_NAND_MAX_ONFI_TIMING_MODE */
	for (i = AL_NAND_MAX_ONFI_TIMING_MODE ; i >= 0 ; i--) {
		if ((1 << i) & async_timing_mode) {
			/*
			 * Addressing RMN: 2903
			 */
			device_properties->timingMode =
					AL_NAND_DEVICE_TIMING_MODE_MANUAL;

			memcpy(&device_properties->timing,
			       &al_nand_manual_timing[i],
			       sizeof(struct al_nand_device_timing));

			break;
		}
	}

	BUG_ON(i < 0);

	nand_set_timing_mode(nand->priv, device_properties->timingMode);

	device_properties->num_col_cyc =
		((nand->onfi_params.addr_cycles & 0xf0) >> 4);
	device_properties->num_row_cyc =
		(nand->onfi_params.addr_cycles & 0x0f);

	onfi_page_size =
		page_size_bytes_convert(nand->onfi_params.byte_per_page);
	device_properties->pageSize = onfi_page_size;

#ifdef CONFIG_AL_NAND_ECC_SUPPORT
	if (nand->onfi_params.ecc_bits == 1) {
		ecc_config->algorithm = AL_NAND_ECC_ALGORITHM_HAMMING;
	} else if (nand->onfi_params.ecc_bits > 1) {
		ecc_config->algorithm = AL_NAND_ECC_ALGORITHM_BCH;
		ecc_config->num_corr_bits =
			bch_num_bits_convert(nand->onfi_params.ecc_bits);
	}
#endif
}

static void nand_ecc_config(
		struct nand_chip *nand,
		struct nand_ecc_ctrl *ecc,
		struct nand_ecclayout *layout,
		uint32_t oob_size,
		uint32_t hw_ecc_enabled,
		uint32_t ecc_loc)
{
#ifdef CONFIG_AL_NAND_ECC_SUPPORT
	if (hw_ecc_enabled != 0) {
		ecc->mode = NAND_ECC_HW;

		memset(layout, 0, sizeof(struct nand_ecclayout));
		layout->eccbytes = oob_size - ecc_loc;
		layout->oobfree[0].offset = 2;
		layout->oobfree[0].length = ecc_loc - 2;
		layout->eccpos[0] = ecc_loc;

		ecc->layout = layout;

		ecc->read_page = ecc_read_page;
		ecc->read_subpage = ecc_read_subpage;
		ecc->write_page = ecc_write_page;

		ecc->strength = nand->onfi_params.ecc_bits;
	} else {
		ecc->mode = NAND_ECC_NONE;
	}
#else
	ecc->mode = NAND_ECC_NONE;
#endif
}

int al_board_nand_init(struct nand_chip *nand, uint32_t devnum)
{
	struct mtd_info *mtd;
	struct nand_data *nand_dat;
	struct al_nand_dev_properties device_properties;
	struct al_nand_extra_dev_properties dev_ext_props;
	const struct fdt_property *fdt_prop;
	const char *prop;
	const u32 *cell;
	void __iomem *nand_base;
	int ret = 0;
	int off;
	int len;

	off = fdt_path_offset(working_fdt, "/soc/nand-flash");
	if (off < 0) {
		debug("%s: nand-flash node not found!\n", __func__);
		return -1;
	}

	prop = (char *)fdt_getprop(working_fdt, off, "status", NULL);
	if (prop && !strcmp(prop, "disabled")) {
		debug("nand unit disabled.\n");
		return 0;
	}

	fdt_prop = fdt_get_property(working_fdt, off, "reg", &len);
	if (!fdt_prop) {
		debug("%s: property '%s' missing\n", __func__, "reg");
		return 0;
	}
	cell = (u32 *)fdt_prop->data;
	/* first cell is reserved for 64 bit address - currently must be 0 */
	BUG_ON(fdt32_to_cpu(*cell) != 0);
	BUG_ON(len < 2);
	cell++;

	nand_base = (void __iomem *)(uintptr_t)fdt32_to_cpu(*cell);

	nand_dat = (struct nand_data *)malloc(sizeof(struct nand_data));
	if (nand_dat == NULL) {
		printf("Failed to allocate nand_data!\n");
		return -1;
	}

	nand_dat->cache_pos = -1;
	nand->priv = nand_dat;

	ret = al_nand_init(&nand_dat->nand_obj,
			nand_base,
			NULL,
			0);

	if (ret != 0) {
		printf("nand init failed\n");
		return ret;
	}

	if (0 != al_nand_dev_config_basic(&nand_dat->nand_obj)) {
		printf("dev_config_basic failed\n");
		return -1;
	}

	nand->options = NAND_NO_SUBPAGE_WRITE;

	nand->cmd_ctrl = nand_cmd_ctrl;
	nand->read_byte = nand_read_byte_from_fifo;
	nand->read_word = nand_read_word;
	nand->read_buf = nand_read_buff;
	nand->dev_ready = nand_dev_ready;
	nand->init_size = nand_init_size;
	nand->write_buf = nand_write_buff;
	nand->select_chip = nand_dev_select;

#ifdef CONFIG_AL_NAND_SIMULATE
	nand->scan_bbt = nand_scan_bbt_simulate;
#endif

	if (0 != al_nand_properties_decode(
					(void __iomem *)AL_PBS_REGFILE_BASE,
					&device_properties,
					&nand_dat->ecc_config,
					&dev_ext_props)) {
		printf("nand_properties_decode failed\n");
		return -1;
	}

	/* must be set before scan_ident cause it uses read_buff */
	nand->ecc.size = 512 << nand_dat->ecc_config.messageSize;
	nand_dat->cw_size = 512 << nand_dat->ecc_config.messageSize;

	mtd = &nand_info[devnum];
	mtd->priv = nand;

	ret = nand_scan_ident(mtd, CONFIG_SYS_NAND_MAX_CHIPS, NULL);
	if (ret)
		return ret;

	nand_onfi_config_set(nand, &device_properties, &nand_dat->ecc_config);

	nand_ecc_config(
		nand,
		&nand->ecc,
		&nand_dat->nand_oob,
		mtd->oobsize,
		dev_ext_props.eccIsEnabled,
		(nand_dat->ecc_config.spareAreaOffset -
				dev_ext_props.pageSize));

	if (0 != al_nand_dev_config(
				&nand_dat->nand_obj,
				&device_properties,
				&nand_dat->ecc_config)) {
		printf("dev_config failed\n");
		return -1;
	}

	ret = nand_scan_tail(mtd);
	if (ret)
		return ret;

	ret = nand_register(devnum);
	if (ret)
		return ret;

	return 0;
}

void board_nand_init()
{
	struct nand_chip *nand = &nand_chip[0];

	if (al_board_nand_init(nand, 0))
		printf("NAND init failed\n");
}

void al_nand_max_onfi_timing_mode_set(unsigned int max_onfi_timing_mode)
{
	debug("%s(%u)\n", __func__, max_onfi_timing_mode);
	al_nand_max_onfi_timing_mode = max_onfi_timing_mode;
}

