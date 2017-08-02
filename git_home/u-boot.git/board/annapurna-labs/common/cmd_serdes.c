 /*
   * board/annapurna-labs/common/cmd_serdes.c
   *
   * Thie file contains a U-Boot command for reading and writing SERDES
   * registers
   *
   * Copyright (C) 2012 Annapurna Labs Ltd.
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

#include "al_globals.h"

#define MASK(msb, lsb)			\
	(((1 << (1 + (msb) - (lsb))) - 1) << (lsb))
#define FIELD_GET(data, msb, lsb)	\
	(((data) & MASK(msb, lsb)) >> (lsb))
#define FIELD_SET(data, msb, lsb, val)	\
	(((data) & ~MASK(msb, lsb)) | (((val) << (lsb)) & MASK(msb, lsb)))

static int parse_reg_page(
	const char		*page_str,
	enum al_serdes_reg_page	*page)
{
	if (!strcmp(page_str, "p0"))
		*page = AL_SRDS_REG_PAGE_0_LANE_0;
	else if (!strcmp(page_str, "p1"))
		*page = AL_SRDS_REG_PAGE_1_LANE_1;
	else if (!strcmp(page_str, "p2"))
		*page = AL_SRDS_REG_PAGE_2_LANE_2;
	else if (!strcmp(page_str, "p3"))
		*page = AL_SRDS_REG_PAGE_3_LANE_3;
	else if (!strcmp(page_str, "p4"))
		*page = AL_SRDS_REG_PAGE_4_COMMON;
	else if (!strcmp(page_str, "p0123"))
		*page = AL_SRDS_REG_PAGE_0123_LANES_0123;
	else {
		printf("Syntax error - invalid reg page!\n\n");
		return -1;
	}

	return 0;
}

static int parse_reg_type(
	const char		*type_str,
	enum al_serdes_reg_type	*type)
{
	if (!strcmp(type_str, "pcs"))
		*type = AL_SRDS_REG_TYPE_PCS;
	else if (!strcmp(type_str, "pma"))
		*type = AL_SRDS_REG_TYPE_PMA;
	else {
		printf("Syntax error - invalid reg type!\n\n");
		return -1;
	}

	return 0;
}

static int parse_lane(
	const char		*lane_str,
	enum al_serdes_lane	*lane)
{
	if (!strcmp(lane_str, "0"))
		*lane = AL_SRDS_LANE_0;
	else if (!strcmp(lane_str, "1"))
		*lane = AL_SRDS_LANE_1;
	else if (!strcmp(lane_str, "2"))
		*lane = AL_SRDS_LANE_2;
	else if (!strcmp(lane_str, "3"))
		*lane = AL_SRDS_LANE_3;
	else if (!strcmp(lane_str, "0123"))
		*lane = AL_SRDS_LANES_0123;
	else {
		printf("Syntax error - invalid lane!\n\n");
		return -1;
	}

	return 0;
}

static int parse_lb_mode(
	const char		*lb_mode_str,
	enum al_serdes_lb_mode	*lb_mode)
{
	if (!strcmp(lb_mode_str, "0"))
		*lb_mode = AL_SRDS_LB_MODE_OFF;
	else if (!strcmp(lb_mode_str, "1"))
		*lb_mode = AL_SRDS_LB_MODE_PMA_IO_UN_TIMED_RX_TO_TX;
	else if (!strcmp(lb_mode_str, "2"))
		*lb_mode = AL_SRDS_LB_MODE_PMA_INTERNALLY_BUFFERED_SERIAL_TX_TO_RX;
	else if (!strcmp(lb_mode_str, "3"))
		*lb_mode = AL_SRDS_LB_MODE_PMA_SERIAL_TX_IO_TO_RX_IO;
	else if (!strcmp(lb_mode_str, "4"))
		*lb_mode = AL_SRDS_LB_MODE_PMA_PARALLEL_RX_TO_TX;
	else {
		printf("Syntax error - invalid loopback mode!\n\n");
		return -1;
	}

	return 0;
}

static int parse_bist_pattern(
	const char			*bist_pattern_str,
	enum al_serdes_bist_pattern	*bist_pattern)
{
	if (!strcmp(bist_pattern_str, "0"))
		*bist_pattern = AL_SRDS_BIST_PATTERN_PRBS7;
	else if (!strcmp(bist_pattern_str, "1"))
		*bist_pattern = AL_SRDS_BIST_PATTERN_PRBS23;
	else if (!strcmp(bist_pattern_str, "2"))
		*bist_pattern = AL_SRDS_BIST_PATTERN_PRBS31;
	else if (!strcmp(bist_pattern_str, "3"))
		*bist_pattern = AL_SRDS_BIST_PATTERN_CLK1010;
	else {
		printf("Syntax error - invalid loopback mode!\n\n");
		return -1;
	}

	return 0;
}

static int parse_rate(
	const char		*rate_str,
	enum al_serdes_rate	*rate)
{
	if (!strcmp(rate_str, "1_8"))
		*rate = AL_SRDS_RATE_1_8;
	else if (!strcmp(rate_str, "1_4"))
		*rate = AL_SRDS_RATE_1_4;
	else if (!strcmp(rate_str, "1_2"))
		*rate = AL_SRDS_RATE_1_2;
	else if (!strcmp(rate_str, "1_1"))
		*rate = AL_SRDS_RATE_FULL;
	else {
		printf("Syntax error - invalid rate!\n\n");
		return -1;
	}

	return 0;
}

static int parse_pm(
	const char		*pm_str,
	enum al_serdes_pm	*pm)
{
	if (!strcmp(pm_str, "pd"))
		*pm = AL_SRDS_PM_PD;
	else if (!strcmp(pm_str, "p2"))
		*pm = AL_SRDS_PM_P2;
	else if (!strcmp(pm_str, "p1"))
		*pm = AL_SRDS_PM_P1;
	else if (!strcmp(pm_str, "p0s"))
		*pm = AL_SRDS_PM_P0S;
	else if (!strcmp(pm_str, "p0"))
		*pm = AL_SRDS_PM_P0;
	else {
		printf("Syntax error - invalid pm!\n\n");
		return -1;
	}

	return 0;
}

static int do_serdes_reg_read(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *page_str;
	const char *type_str;
	const char *offset_str;
	const char *msb_str;
	const char *lsb_str;

	enum al_serdes_reg_page page;
	enum al_serdes_reg_type type;
	int offset;
	int msb;
	int lsb;

	uint8_t data;
	int err;

	if (argc != 6) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	page_str = argv[1];
	type_str = argv[2];
	offset_str = argv[3];
	msb_str = argv[4];
	lsb_str = argv[5];

	if (parse_reg_page(page_str, &page))
		return -1;

	if (page == AL_SRDS_REG_PAGE_0123_LANES_0123) {
		printf("Syntax error - invalid page!\n\n");
		return -1;
	}

	if (parse_reg_type(type_str, &type))
		return -1;

	offset = simple_strtol(offset_str, NULL, 10);
	if (offset < 0) {
		printf("Syntax error - invalid offset!\n\n");
		return -1;
	}

	msb = simple_strtol(msb_str, NULL, 10);
	if (msb < 0) {
		printf("Syntax error - invalid msb!\n\n");
		return -1;
	}

	lsb = simple_strtol(lsb_str, NULL, 10);
	if (lsb < 0) {
		printf("Syntax error - invalid lsb!\n\n");
		return -1;
	}

	err = al_serdes_reg_read(
		&al_globals.serdes,
		grp,
		page,
		type,
		offset,
		&data);
	if (err) {
		printf(
			"al_serdes_reg_read failed!\n\n");
		return -1;
	}

	printf(
		"SERDES %d, %s, %s, reg[%d][%d:%d] => 0x%02x\n",
		grp,
		page_str,
		type_str,
		offset,
		msb,
		lsb,
		FIELD_GET(data, msb, lsb));

	return 0;
}

static int do_serdes_reg_write(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *page_str;
	const char *type_str;
	const char *offset_str;
	const char *msb_str;
	const char *lsb_str;
	const char *val_str;

	enum al_serdes_reg_page page;
	enum al_serdes_reg_type type;
	int offset;
	int msb;
	int lsb;
	int val;

	uint8_t data;
	int err;

	if (argc != 7) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	page_str = argv[1];
	type_str = argv[2];
	offset_str = argv[3];
	msb_str = argv[4];
	lsb_str = argv[5];
	val_str = argv[6];

	if (parse_reg_page(page_str, &page)) {
		printf("Syntax error - invalid page!\n\n");
		return -1;
	}

	if (parse_reg_type(type_str, &type)) {
		printf("Syntax error - invalid type!\n\n");
		return -1;
	}

	offset = simple_strtol(offset_str, NULL, 10);
	if (offset < 0) {
		printf("Syntax error - invalid offset!\n\n");
		return -1;
	}

	msb = simple_strtol(msb_str, NULL, 10);
	if (msb < 0) {
		printf("Syntax error - invalid msb!\n\n");
		return -1;
	}

	lsb = simple_strtol(lsb_str, NULL, 10);
	if (lsb < 0) {
		printf("Syntax error - invalid lsb!\n\n");
		return -1;
	}

	val = simple_strtol(val_str, NULL, 16);
	if ((val < 0) || (val > 0xff)) {
		printf("Syntax error - invalid value!\n\n");
		return -1;
	}

	if (page == AL_SRDS_REG_PAGE_0123_LANES_0123
		&& (msb != 7 || lsb != 0)) {
		printf("Syntax error - p0123 can only be used with msb=7 and lsb=0!\n\n");
		return -1;
	}

	if (page != AL_SRDS_REG_PAGE_0123_LANES_0123) {
		err = al_serdes_reg_read(
			&al_globals.serdes,
			grp,
			page,
			type,
			offset,
			&data);
		if (err) {
			printf(
				"al_serdes_reg_read failed!\n\n");
			return -1;
		}
	} else {
		data = 0;
	}

	data = FIELD_SET(data, msb, lsb, val);

	err = al_serdes_reg_write(
		&al_globals.serdes,
		grp,
		page,
		type,
		offset,
		data);
	if (err) {
		printf(
			"al_serdes_reg_write failed!\n\n");
		return -1;
	}

	printf(
		"SERDES %d, %s, %s, reg[%d][%d:%d] <= 0x%02x\n",
		grp,
		page_str,
		type_str,
		offset,
		msb,
		lsb,
		val);

	return 0;
}

static int do_serdes_pma_reset_grp_en(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *en_str;

	int en;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	en_str = argv[1];

	en = simple_strtol(en_str, NULL, 10);

	al_serdes_pma_hard_reset_group(
		&al_globals.serdes,
		grp,
		en);

	return 0;
}

static int do_serdes_pma_reset_lane_en(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *lane_str;
	const char *en_str;

	enum al_serdes_lane lane;
	int en;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];
	en_str = argv[2];

	if (parse_lane(lane_str, &lane))
		return -1;

	en = simple_strtol(en_str, NULL, 10);

	al_serdes_pma_hard_reset_lane(
		&al_globals.serdes,
		grp,
		lane,
		en);

	return 0;
}

static int do_serdes_bist_overrides(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *rate_str;
	enum al_serdes_rate rate;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	rate_str = argv[1];

	if (parse_rate(rate_str, &rate))
		return -1;

	al_serdes_bist_overrides_enable(&al_globals.serdes, grp, rate);

	return 0;
}

static int do_serdes_lane_pm_set(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *lane_str;
	const char *pm_str;

	enum al_serdes_lane lane;
	enum al_serdes_pm pm;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];
	pm_str = argv[2];

	if (parse_lane(lane_str, &lane))
		return -1;

	if (parse_pm(pm_str, &pm))
		return -1;

	al_serdes_lane_pm_set(&al_globals.serdes, grp, lane, pm, pm);

	return 0;
}

static int do_serdes_group_pm_set(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *pm_str;

	enum al_serdes_pm pm;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	pm_str = argv[1];

	if (parse_pm(pm_str, &pm))
		return -1;

	al_serdes_group_pm_set(&al_globals.serdes, grp, pm);

	return 0;
}

static int do_serdes_lb(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *lane_str;
	const char *lb_mode_str;

	enum al_serdes_lane lane;
	enum al_serdes_lb_mode lb_mode;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];
	lb_mode_str = argv[2];

	if (parse_lane(lane_str, &lane))
		return -1;

	if (parse_lb_mode(lb_mode_str, &lb_mode))
		return -1;

	al_serdes_loopback_control(
		&al_globals.serdes,
		grp,
		lane,
		lb_mode);

	return 0;
}

static int do_serdes_bist_pat(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *bist_pattern_str;

	enum al_serdes_bist_pattern bist_pattern;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	bist_pattern_str = argv[1];

	if (parse_bist_pattern(bist_pattern_str, &bist_pattern))
		return -1;

	al_serdes_bist_pattern_select(
		&al_globals.serdes,
		grp,
		bist_pattern,
		NULL);

	return 0;
}

static int do_serdes_bist_tx_en(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *lane_str;
	const char *en_str;

	enum al_serdes_lane lane;
	int en;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];
	en_str = argv[2];

	if (parse_lane(lane_str, &lane))
		return -1;

	en = simple_strtol(en_str, NULL, 10);

	al_serdes_bist_tx_enable(
		&al_globals.serdes,
		grp,
		lane,
		en);

	return 0;
}

static int do_serdes_bist_tx_err(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	if (argc != 1) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	al_serdes_bist_tx_err_inject(
		&al_globals.serdes,
		grp);

	return 0;
}

static int do_serdes_bist_rx_en(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *lane_str;
	const char *en_str;

	enum al_serdes_lane lane;
	int en;

	if (argc != 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];
	en_str = argv[2];

	if (parse_lane(lane_str, &lane))
		return -1;

	en = simple_strtol(en_str, NULL, 10);

	al_serdes_bist_rx_enable(
		&al_globals.serdes,
		grp,
		lane,
		en);

	return 0;
}

static int do_serdes_bist_rx_stat(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	const char *lane_str;

	enum al_serdes_lane lane;

	al_bool is_locked;
	al_bool err_cnt_overflow;
	uint16_t err_cnt;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];

	if (parse_lane(lane_str, &lane))
		return -1;

	al_serdes_bist_rx_status(
		&al_globals.serdes,
		grp,
		lane,
		&is_locked,
		&err_cnt_overflow,
		&err_cnt);

	printf(
		"SERDES %d, %d BIST RX status: is_locked=%d, "
		"err_cnt_overflow=%d err_cnt=%d\n",
		grp,
		lane,
		is_locked,
		err_cnt_overflow,
		err_cnt);

	return 0;
}

static void print_tx_params(
		enum al_serdes_group            grp,
		enum al_serdes_lane      	lane,
		struct al_serdes_adv_tx_params *tx_params)
{
	printf("group          = %d\n", grp);
	printf("lane           = %d\n", lane);
	printf("amplitude      = %d\n", tx_params->amp);
	printf("drivers number = %d\n", tx_params->total_driver_units);
	printf("post emphasis  = %d\n", tx_params->c_plus_1);
	printf("pre emphasis   = %d\n", tx_params->c_minus_1);
	printf("slew rate      = %d\n", tx_params->slew_rate);
	printf("c_plus_2       = %d\n", tx_params->c_plus_2);
	printf("for setting Tx params manually, use the following line:\n");
	printf("%d %d %d %d %d %d %d\n", grp, lane,
					 tx_params->amp,
					 tx_params->total_driver_units,
					 tx_params->c_plus_1,
					 tx_params->c_minus_1,
					 tx_params->slew_rate);
}

static int do_serdes_tx_params_set(
		enum al_serdes_group	grp,
		int			argc,
		char *const		argv[])
{
	const char *lane_str        = argv[1];
	const char *amp_str         = argv[2];
	const char *drivers_num_str = argv[3];
	const char *post_emph_str   = argv[4];
	const char *pre_emph_str    = argv[5];
	const char *slew_rate_str   = argv[6];

	enum al_serdes_lane lane;
	struct al_serdes_adv_tx_params tx_params;
	int val;

	if (argc != 7) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	tx_params.override = AL_TRUE;

	if (parse_lane(lane_str, &lane)) {
		printf("Syntax error - invalid lane!\n\n");
		return -1;
	}


	val = simple_strtol(amp_str, NULL, 10);
	if (val < 0 || val > 7) {
		printf("Syntax error - invalid amplitude!\n\n");
		return -1;
	}
	tx_params.amp = val;

	val = simple_strtol(drivers_num_str, NULL, 10);
	if (val < 0) {
		printf("Syntax error - invalid 'drivers number'!\n\n");
		return -1;
	}
	tx_params.total_driver_units = val;

	val = simple_strtol(post_emph_str, NULL, 10);
	if (val < 0 || val > 9) {
		printf("Syntax error - invalid 'post emphasis'!\n\n");
		return -1;
	}
	tx_params.c_plus_1 = val;

	val = simple_strtol(pre_emph_str, NULL, 10);
	if (val < 0 || val > 6) {
		printf("Syntax error - invalid 'pre emphasis'!\n\n");
		return -1;
	}
	tx_params.c_minus_1 = val;

	val = simple_strtol(slew_rate_str, NULL, 10);
	if (val < 0 || val > 3) {
		printf("Syntax error - invalid 'slew rate'!\n\n");
		return -1;
	}
	tx_params.slew_rate = val;

	tx_params.c_plus_2 = 0;

	al_serdes_tx_advanced_params_set(
			&al_globals.serdes, grp, lane, &tx_params);

	return 0;
}

static int do_serdes_tx_params_get(
		enum al_serdes_group	grp,
		int			argc,
		char *const		argv[])
{
	struct al_serdes_adv_tx_params tx_params;
	enum al_serdes_lane lane;
	const char *lane_str = argv[1];

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	if (parse_lane(lane_str, &lane) || lane >= AL_SRDS_NUM_LANES) {
		printf("Syntax error - invalid lane!\n\n");
		return -1;
	}

	al_serdes_tx_advanced_params_get(
			&al_globals.serdes, grp, lane, &tx_params);

	print_tx_params(grp, lane, &tx_params);

	return 0;
}

static void print_rx_params(
		enum al_serdes_group	          grp,
		enum al_serdes_lane	          lane,
		struct al_serdes_adv_rx_params* rx_params)
{
	printf("dc gain = %02x\n",           rx_params->dcgain);
	printf("dfe ps tap 3db = %02x\n",    rx_params->dfe_3db_freq);
	printf("dfe ps tap gain = %02x\n",   rx_params->dfe_gain);
	printf("dfe tap1 gain = %02x\n",     rx_params->dfe_first_tap_ctrl);
	printf("dfe tap2 gain = %02x\n",     rx_params->dfe_secound_tap_ctrl);
	printf("dfe tap3 gain = %02x\n",     rx_params->dfe_third_tap_ctrl);
	printf("dfe tap4 gain = %02x\n",     rx_params->dfe_fourth_tap_ctrl);
	printf("low freq agc gain = %02x\n", rx_params->low_freq_agc_gain);
	printf("high freq agc cap = %02x\n", rx_params->high_freq_agc_boost);

	printf("for setting Rx params manually, use the following line:\n");
	printf("%d %d %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		grp, lane,
		rx_params->dcgain, rx_params->dfe_3db_freq,
		rx_params->dfe_gain, rx_params->dfe_first_tap_ctrl,
		rx_params->dfe_secound_tap_ctrl, rx_params->dfe_third_tap_ctrl,
		rx_params->dfe_fourth_tap_ctrl, rx_params->low_freq_agc_gain,
		rx_params->high_freq_agc_boost);
}

static int do_serdes_rx_eq(
		enum al_serdes_group	grp,
		int			argc,
		char *const		argv[])
{
	enum al_serdes_lane  lane;
	struct al_serdes_adv_rx_params rx_params;
	int best_score  = -1;
	int best_width  = -1;
	int best_height = -1;
	int i;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}
	if (parse_lane(argv[1], &lane))
		return -1;

	for(i = 0; i < 10; ++i) {
		int test_score;
		int test_width  = -1;
		int test_height = -1;
		test_score = al_serdes_rx_equalization(
				&al_globals.serdes, grp, lane);
		if(test_score >= best_score)
			al_serdes_calc_eye_size(
					&al_globals.serdes, grp, lane,
					&test_width, &test_height);

		if ((test_score > best_score) ||
			(test_score == best_score && test_width > best_width)||
			(test_score == best_score && test_width == best_width &&
					test_height > best_height)) {
			best_score  = test_score;
			best_width  = test_width;
			best_height = test_height;
			al_serdes_rx_advanced_params_get(
				&al_globals.serdes, grp, lane, &rx_params);
		}
	}

	if(best_score == -1) {
		printf("rx equalization failed with each try, "
				"no suitable output\n");
	} else {
		rx_params.precal_code_sel = 0;
		rx_params.override = AL_TRUE;
		al_serdes_rx_advanced_params_set(
				&al_globals.serdes, grp, lane, &rx_params);

		printf("best test score is: %d\n", best_score);
		print_rx_params(grp, lane, &rx_params);
	}

	return 0;
}

static int do_serdes_rx_params_set(
		enum al_serdes_group	grp,
		int			argc,
		char *const		argv[])
{
	enum al_serdes_lane  lane;
	struct al_serdes_adv_rx_params rx_params;

	const char* cmd_arg[] = { "grp",
				  "lane",
				  "dc gain",
				  "dfe ps tap 3db",
				  "dfe ps tap gain",
				  "dfe tap1 gain",
				  "dfe tap2 gain",
				  "dfe tap3 gain",
				  "dfe tap4 gain",
				  "low freq agc gain",
				  "high freq agc cap" };
	int err = 0;
	if (argc != 11) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	if (parse_lane(argv[1], &lane))
		return -1;

	rx_params.dcgain               = simple_strtol(argv[2],  NULL, 16);
	rx_params.dfe_3db_freq         = simple_strtol(argv[3],  NULL, 16);
	rx_params.dfe_gain             = simple_strtol(argv[4],  NULL, 16);
	rx_params.dfe_first_tap_ctrl   = simple_strtol(argv[5],  NULL, 16);
	rx_params.dfe_secound_tap_ctrl = simple_strtol(argv[6],  NULL, 16);
	rx_params.dfe_third_tap_ctrl   = simple_strtol(argv[7],  NULL, 16);
	rx_params.dfe_fourth_tap_ctrl  = simple_strtol(argv[8],  NULL, 16);
	rx_params.low_freq_agc_gain    = simple_strtol(argv[9],  NULL, 16);
	rx_params.high_freq_agc_boost  = simple_strtol(argv[10], NULL, 16);

	if (rx_params.dcgain > 7)
		err = 2;
	else if (rx_params.dfe_3db_freq > 7)
		err = 3;
	else if (rx_params.dfe_gain > 7)
		err = 4;
	else if (rx_params.dfe_first_tap_ctrl > 15)
		err = 5;
	else if (rx_params.dfe_secound_tap_ctrl > 15)
		err = 6;
	else if (rx_params.dfe_third_tap_ctrl > 15)
		err = 7;
	else if (rx_params.dfe_fourth_tap_ctrl >15)
		err = 8;
	else if (rx_params.low_freq_agc_gain > 15)
		err = 9;
	else if (rx_params.high_freq_agc_boost >31)
		err = 10;

	if (err) {
		printf("Syntax error - invalid number %s of parameter %s",
				argv[err], cmd_arg[err]);
		return -1;
	}

	rx_params.precal_code_sel = 0;
	rx_params.override = AL_TRUE;
	al_serdes_rx_advanced_params_set(
			&al_globals.serdes, grp, lane, &rx_params);

	return 0;
}

static int do_serdes_rx_params_get(
		enum al_serdes_group	grp,
		int			argc,
		char *const		argv[])
{
	enum al_serdes_lane  lane;
	struct al_serdes_adv_rx_params rx_params;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	if (parse_lane(argv[1], &lane))
		return -1;

	al_serdes_rx_advanced_params_get(
			&al_globals.serdes, grp, lane, &rx_params);
	print_rx_params(grp, lane, &rx_params);

	return 0;
}

int do_serdes_eye_diag(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	enum al_serdes_lane lane;
	int x;
	int y;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	if (parse_lane(argv[1], &lane))
		return -1;

	printf("       ------------------------------------------------------------------\n");

	for (y = 0; y < 63; y++) {
		if (((y - 1) % 6) == 0)
			printf("%+4dmV-+", ((31 - y) * 483) / 30);
		else
			printf("       |");

		for (x = 0 ; x < 64; x++) {
			int err;
			unsigned int err_cnt;

			err = al_serdes_eye_diag_sample(
				&al_globals.serdes, grp, lane, x, y, 1000, &err_cnt);
			if (err) {
				printf("al_serdes_eye_diag_sample failed!\n\n");
				return -1;
			}

			if (err_cnt == 0)
				printf(" ");
			else if (err_cnt < 10)
				printf("1");
			else if (err_cnt < 100)
				printf("2");
			else if (err_cnt < 1000)
				printf("3");
			else if (err_cnt < 10000)
				printf("4");
			else
				printf("5");
		}

		printf("|\n");
	}

	printf("        +-------+-------+-------+-------+-------+-------+-------+-------+\n");
	printf("        |       |       |       |       |       |       |       |       |\n");
	printf("    -1.00UI -0.75UI -0.50UI -0.25UI +0.00UI +0.25UI +0.50UI +0.75UI +1.00UI\n\n");
	printf("' ' - No errors\n");
	printf("'1' - Less than 10 errors\n");
	printf("'2' - Less than 100 errors\n");
	printf("'3' - Less than 1000 errors\n");
	printf("'4' - Less than 10000 errors\n");
	printf("'5' - More than 10000 errors\n\n");

	return 1;
}

static int do_serdes_rx_scan(
	enum al_serdes_group	grp,
	int			argc,
	char *const		argv[])
{
	//assumption PRBS and serdes already configured.
	const char *lane_str;
	enum al_serdes_lane lane;
	int HF_AGC_BOOST, LF_AGC_GAIN/*, DFE_TAP4, DFE_TAP3, DFE_TAP2, DFE_TAP1*/;
	int DFE_POST_SHAPING, DFE_POST_3DB, HF_AGC_GAIN;
	uint8_t data;
	int err;
	al_bool is_locked;
	al_bool err_cnt_overflow;
	uint16_t err_cnt;


	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	lane_str = argv[1];

	if (parse_lane(lane_str, &lane))
		return -1;
	enum al_serdes_reg_page	page;
	if (lane == 0) page = AL_SRDS_REG_PAGE_0_LANE_0;
	else if (lane == 1) page = AL_SRDS_REG_PAGE_1_LANE_1;
	else if (lane == 2) page = AL_SRDS_REG_PAGE_2_LANE_2;
	else page = AL_SRDS_REG_PAGE_3_LANE_3;


	//NO DFE -- For now.
	for (HF_AGC_BOOST = 0 ; HF_AGC_BOOST < 32 ;  HF_AGC_BOOST ++){
		for (LF_AGC_GAIN = 0 ; LF_AGC_GAIN < 8 ; LF_AGC_GAIN ++){
			for (HF_AGC_GAIN = 0 ; HF_AGC_GAIN < 8 ; HF_AGC_GAIN ++){
				for (DFE_POST_SHAPING = 0 ; DFE_POST_SHAPING < 8 ; DFE_POST_SHAPING ++){
					for (DFE_POST_3DB = 0 ; DFE_POST_3DB < 8 ; DFE_POST_3DB ++){
						//Set params
						err = al_serdes_reg_read(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							24,
							&data);
						if (err) {
							printf(
								"al_serdes_reg_read failed!\n\n");
							return -1;
						}
						data = FIELD_SET(data, 2, 0,HF_AGC_GAIN );
						data = FIELD_SET(data, 5, 3,DFE_POST_3DB );
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							24,
							data);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}

						err = al_serdes_reg_read(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							25,
							&data);
						if (err) {
							printf(
								"al_serdes_reg_read failed!\n\n");
							return -1;
						}
						data = FIELD_SET(data, 2, 0,DFE_POST_SHAPING );
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							25,
							data);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}

						err = al_serdes_reg_read(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							27,
							&data);
						if (err) {
							printf(
								"al_serdes_reg_read failed!\n\n");
							return -1;
						}
						data = FIELD_SET(data, 6, 4,LF_AGC_GAIN );
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							27,
							data);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}

						err = al_serdes_reg_read(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							28,
							&data);
						if (err) {
							printf(
								"al_serdes_reg_read failed!\n\n");
							return -1;
						}
						data = FIELD_SET(data, 7, 3,HF_AGC_BOOST );
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							28,
							data);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
						//Setting is done, Do PMA-ADDQ -> PMA P0
//serdes wr 0 p0123 pma 4 7 0 10
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							4,
							0x10);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
//serdes wr 0 p0123 pma 3 7 0 1
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							3,
							1);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
//serdes wr 0 p4 pma 3 7 0 1
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							AL_SRDS_REG_PAGE_4_COMMON,
							AL_SRDS_REG_TYPE_PMA,
							3,
							1);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
//serdes wr 0 p0123 pma 4 7 0 10
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							4,
							0x10);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
//serdes wr 0 p0123 pma 3 7 0 10
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							page,
							AL_SRDS_REG_TYPE_PMA,
							3,
							0x10);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
//serdes wr 0 p4 pma 3 7 0 10
						err = al_serdes_reg_write(
							&al_globals.serdes,
							grp,
							AL_SRDS_REG_PAGE_4_COMMON,
							AL_SRDS_REG_TYPE_PMA,
							3,
							0x10);
						if (err) {
							printf(
								"al_serdes_reg_write failed!\n\n");
							return -1;
						}
						// Now disable and re enable bist
						al_serdes_bist_rx_enable(
							&al_globals.serdes,
							grp,
							lane,
							0);
						al_serdes_bist_rx_enable(
							&al_globals.serdes,
							grp,
							lane,
							1);
						// Now Check Status
						udelay(50000);
						al_serdes_bist_rx_status(
							&al_globals.serdes,
							grp,
							lane,
							&is_locked,
							&err_cnt_overflow,
							&err_cnt);
						//If every this is fine, print values
							if ((is_locked) && (err_cnt == 0) && (err_cnt_overflow == 0)){
								printf("HF_AGC_BOOST = 0x%x, LF_AGC_GAIN=0x%x, HF_AGC_GAIN=0x%x, DFE_POST_SHAPING=0x%x, DFE_POST_3DB=0x%x\n" ,HF_AGC_BOOST,LF_AGC_GAIN,HF_AGC_GAIN, DFE_POST_SHAPING, DFE_POST_3DB);
							}
					}
				}
			}
		}
	}
	return 0;
}

int do_serdes(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	const char *op;
	const char *grp_str;

	enum al_serdes_group grp;

	if (argc < 3) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	op = argv[1];
	grp_str = argv[2];

	grp = simple_strtol(grp_str, NULL, 10);
	if ((grp < AL_SRDS_GRP_A) || (grp >= AL_SRDS_NUM_GROUPS)) {
		printf("Syntax error - invalid group!\n\n");
		return -1;
	}

	argv += 2;
	argc -= 2;

	if (!strcmp(op, "rd"))
		return do_serdes_reg_read(grp, argc, argv);
	else if (!strcmp(op, "wr"))
		return do_serdes_reg_write(grp, argc, argv);
	else if (!strcmp(op, "pma_reset_grp_en"))
		return do_serdes_pma_reset_grp_en(grp, argc, argv);
	else if (!strcmp(op, "pma_reset_lane_en"))
		return do_serdes_pma_reset_lane_en(grp, argc, argv);
	else if (!strcmp(op, "pm_grp_set"))
		return do_serdes_group_pm_set(grp, argc, argv);
	else if (!strcmp(op, "pm_lane_set"))
		return do_serdes_lane_pm_set(grp, argc, argv);
	else if (!strcmp(op, "lb"))
		return do_serdes_lb(grp, argc, argv);
	else if (!strcmp(op, "bist_overrides"))
		return do_serdes_bist_overrides(grp, argc, argv);
	else if (!strcmp(op, "bist_pat"))
		return do_serdes_bist_pat(grp, argc, argv);
	else if (!strcmp(op, "bist_tx_en"))
		return do_serdes_bist_tx_en(grp, argc, argv);
	else if (!strcmp(op, "bist_tx_err"))
		return do_serdes_bist_tx_err(grp, argc, argv);
	else if (!strcmp(op, "bist_rx_en"))
		return do_serdes_bist_rx_en(grp, argc, argv);
	else if (!strcmp(op, "bist_rx_stat"))
		return do_serdes_bist_rx_stat(grp, argc, argv);
	else if (!strcmp(op, "eye_diag"))
		return do_serdes_eye_diag(grp, argc, argv);
	else if (!strcmp(op, "rx_scan"))
		return do_serdes_rx_scan(grp, argc, argv);
	else if (!strcmp(op, "tx_params_set"))
		return do_serdes_tx_params_set(grp, argc, argv);
	else if (!strcmp(op, "tx_params_get"))
		return do_serdes_tx_params_get(grp, argc, argv);
	else if (!strcmp(op, "rx_equal"))
		return do_serdes_rx_eq(grp, argc, argv);
	else if (!strcmp(op, "rx_params_set"))
		return do_serdes_rx_params_set(grp, argc, argv);
	else if (!strcmp(op, "rx_params_get"))
		return do_serdes_rx_params_get(grp, argc, argv);
	else {
		printf("Syntax error - invalid op!\n\n");
		return -1;
	}

	return 0;
}

U_BOOT_CMD(
	serdes, 13, 0, do_serdes,
	"Serdes debug",
	"serdes op <arg1> <arg2> ...\n\n"
	" - op - operation:\n"
	"        rd - read a SERDES register\n"
	"             arg1 - grp (see below)\n"
	"             arg2 - reg page (see below)\n"
	"             arg3 - reg type (see below)\n"
	"             arg4 - register offset (decimal)\n"
	"             arg5 - msb (0-7)\n"
	"             arg6 - lsb (0-7)\n"
	"\n"
	"        wr - write a SERDES register\n"
	"             arg1 - grp (see below)\n"
	"             arg2 - reg page (see below)\n"
	"             arg3 - reg type (see below)\n"
	"             arg4 - register offset (decimal)\n"
	"             arg5 - msb (0-7)\n"
	"             arg6 - lsb (0-7)\n"
	"             arg7 - the write value (hex, 0-ff)\n"
	"\n"
	"        pma_reset_grp_en - PMA hard reset group enable/disable\n"
	"                     arg1 - grp (see below)\n"
	"                     arg2 - 1 - enable, 0 - disable\n"
	"\n"
	"        pma_reset_lane_en - PMA hard reset lane enable/disable\n"
	"                     arg1 - grp (see below)\n"
	"                     arg2 - lane (0-3)\n"
	"                     arg3 - 1 - enable, 0 - disable\n"
	"\n"
	"        pm_grp_set - Set power mode of a group (not of its lanes)\n"
	"             arg1 - grp (see below)\n"
	"             arg2 - power mode (pd, p2, p1, p0s, p0)\n"
	"\n"
	"        pm_lane_set - Set power mode of a lane (not of its group)\n"
	"             arg1 - grp (see below)\n"
	"             arg2 - lane (0-3)\n"
	"             arg3 - power mode (pd, p2, p1, p0s, p0)\n"
	"\n"
	"        lb - loopback control\n"
	"             arg1 - grp (see below)\n"
	"             arg2 - lane (0-3)\n"
	"             arg3 - loopback mode:\n"
	"                    0 - No loopback\n"
	"                    1 - Untimed RX to TX\n"
	"                    2 - TX serializer output into the CDR\n"
	"                    3 - TX driver IO signal to the RX IO pins\n"
	"                    4 - PMA RX lane data ports to TX lane data ports\n"
	"\n"
	"        bist_overrides - Enable BIST required overrides\n"
	"             arg1 - grp (see below)\n"
	"             arg2 - SerDes rate:\n"
	"                    1_8 - 1/8 rate\n"
	"                    1_4 - 1/4 rate\n"
	"                    1_2 - 1/2 rate\n"
	"                    1_1 - Full rate\n"
	"\n"
	"        bist_pat - BIST pattern selection\n"
	"                   arg1 - grp (see below)\n"
	"                   arg2 - pattern:\n"
	"                          0 - PRBS 2^7\n"
	"                          1 - PRBS 2^23\n"
	"                          2 - PRBS 2^31\n"
	"                          3 - CLK 1010\n"
	"\n"
	"        bist_tx_en - BIST TX enable/disable\n"
	"                     arg1 - grp (see below)\n"
	"                     arg2 - lane (0-3)\n"
	"                     arg3 - 1 - enable, 0 - disable\n"
	"\n"
	"        bist_tx_err - BIST TX single bit error injection\n"
	"                      arg1 - grp (see below)\n"
	"\n"
	"        bist_rx_en - BIST RX enable/disable\n"
	"                     arg1 - grp (see below)\n"
	"                     arg2 - lane (0-3)\n"
	"                     arg3 - 1 - enable, 0 - disable\n"
	"\n"
	"        bist_rx_stat - BIST RX status information\n"
	"                       arg1 - grp (see below)\n"
	"                       arg2 - lane (0-3)\n"
	"\n"
	"        eye_diag - Eye diagram\n"
	"                       arg1 - grp (see below)\n"
	"                       arg2 - lane (0-3)\n"
	"\n"
	"        tx_params_set -Set Tx Params\n"
	"                        arg1 - grp (see below)\n"
	"                        arg2 - lane (0-3 or 0123 for all)\n"
	"                        arg3 - amplitude:\n"
	"                                   0 - Not Supported\n"
	"                                   1 - 952mVdiff-pkpk\n"
	"                                   2 - 1024mVdiff-pkpk\n"
	"                                   3 - 1094mVdiff-pkpk\n"
	"                                   4 - 1163mVdiff-pkpk\n"
	"                                   5 - 1227mVdiff-pkpk\n"
	"                                   6 - 1283mVdiff-pkpk\n"
	"                                   7 - 1331mVdiff-pkpk\n"
	"                        arg4 - drivers number (decimal: 0 - 31)\n"
	"                        arg5 - post emphasis  (decimal: 0 - 9)\n"
	"                        arg6 - pre emphasis   (decimal: 0 - 6)\n"
	"                        arg7 - slew rate:\n"
	"                                   0 - 31ps\n"
	"                                   1 - 33ps\n"
	"                                   2 - 68ps\n"
	"                                   3 - 170ps\n"
	"\n"
	"        tx_params_get - Get Tx Params\n"
	"                        arg1 - grp (see below)\n"
	"                        arg2 - lane (0-3)\n"
	"\n"
	"        rx_equal- perform SerDes HW equalization several times,\n"
	"                  prints and update parametrs of best resualt\n"
	"                  arg1 - grp (see below)\n"
	"                  arg2 - lane (0-3)\n"
	"\n"
	"        rx_params_set - Set Serdes rx params\n"
	"                        arg1  - grp (see below)\n"
	"                        arg2  - lane (0-3)\n"
	"                        arg3  - dc gain\n"
	"                        arg4  - dfe ps tap 3db\n"
	"                        arg5  - dfe ps tap gain\n"
	"                        arg6  - dfe tap1 gain\n"
	"                        arg7  - dfe tap2 gain\n"
	"                        arg8  - dfe tap3 gain\n"
	"                        arg9  - dfe tap4 gain\n"
	"                        arg10 - low freq agc gain\n"
	"                        arg11 - high freq agc cap\n"
	"\n"
	"        rx_params_get - Set Serdes rx params\n"
	"                        arg1  - grp (see below)\n"
	"                        arg2  - lane (0-3)\n"
	"\n"
	" - grp - the SERDES group: 0-3\n"
	"\n"
	" - reg page - the SERDES page:\n"
	"              p0 - page 0\n"
	"              p1 - page 1\n"
	"              p2 - page 2\n"
	"              p3 - page 3\n"
	"              p4 - page 4 - common\n"
	"              p0123 - pages 0, 1, 2, 3 simultaneously (write only)\n"
	"\n"
	" - reg type - the SERDES register type: pma/pcs\n\n"
);

