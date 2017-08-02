 /*
   * board/annapurna-labs/common/cmd_ddr.c
   *
   * Thie file contains a U-Boot command for DDR related operations
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

#include "al_hal_ddr.h"
#include "al_hal_iomap.h"

static void do_ddr_ecc_stats_print(
	struct al_ddr_ecc_status	*status,
	int				corr)
{
	printf(" - Number of ECC errors detected: %u\n", status->err_cnt);

	if (status->err_cnt) {
		printf(" - First erroneous read address: rank: %u, Bank: %u, Row: %u, "
			"Col: %u\n",
			status->rank, status->bank, status->row, status->col);

		printf(" - First errneous data: "
			"%08x:%08x:%02x\n",
			status->syndromes_63_32, status->syndromes_31_0,
			status->syndromes_ecc);

		if (!corr)
			return;

		printf(" - Mask for the corrected data portion: %08x:%08x:%02x\n",
			status->corr_bit_mask_63_32, status->corr_bit_mask_31_0,
			status->corr_bit_mask_ecc);

		printf(" - Bit number corrected by single-bit ECC error: %u\n",
			status->ecc_corrected_bit_num);
	}
}

int do_ddr_ecc_stats(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	struct al_ddr_cfg ddr_cfg;
	struct al_ddr_ecc_status uncorr_status;
	struct al_ddr_ecc_status corr_status;
	int clear = 0;
	int err;
	struct al_ddr_ecc_cfg ddr_ecc_cfg;

	if (argc > 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "clear")) {
			printf("Syntax error - invalid parameters!\n\n");
			return -1;
		}

		clear = 1;
	}

	err = al_ddr_cfg_init(
		(void __iomem *)AL_NB_SERVICE_BASE,
		(void __iomem *)AL_NB_DDR_CTL_BASE,
		(void __iomem *)AL_NB_DDR_PHY_BASE,
		&ddr_cfg);
	if (err) {
		printf("al_ddr_cfg_init failed!\n");
		return -1;
	}

	al_ddr_ecc_cfg_get(&ddr_cfg, &ddr_ecc_cfg);
	if (ddr_ecc_cfg.ecc_enabled == AL_FALSE) {
		printf("DDR ECC is disabled\n");
		return 0;
	}

	err = al_ddr_ecc_status_get(
		&ddr_cfg, &corr_status, &uncorr_status);
	if (err) {
		printf("al_ddr_ecc_status_get failed!\n");
		return -1;
	}

	printf("Corrected status:\n");
	do_ddr_ecc_stats_print(&corr_status, 1);

	printf("Uncorrected status:\n");
	do_ddr_ecc_stats_print(&uncorr_status, 0);

	if (clear) {
		al_ddr_ecc_corr_count_clear(&ddr_cfg);
		al_ddr_ecc_corr_int_clear(&ddr_cfg);
		al_ddr_ecc_uncorr_count_clear(&ddr_cfg);
		al_ddr_ecc_uncorr_int_clear(&ddr_cfg);
	}

	return 0;
}

int do_ddr_ecc_poison(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	int err;
	unsigned int rank;
	unsigned int bank;
	unsigned int bg;
	unsigned int col;
	unsigned int row;
	unsigned int ddr_addr;
	struct al_ddr_cfg ddr_cfg;

	if (argc != 2) {
		printf("Syntax error - invalid number of parameters!\n\n");
		return -1;
	}

	ddr_addr = simple_strtol(argv[1], NULL, 16);

	err = al_ddr_cfg_init(
		(void __iomem *)AL_NB_SERVICE_BASE,
		(void __iomem *)AL_NB_DDR_CTL_BASE,
		(void __iomem *)AL_NB_DDR_PHY_BASE,
		&ddr_cfg);
	if (err) {
		printf("al_ddr_cfg_init failed!\n");
		return -1;
	}

	if (ddr_addr) {
		err = al_ddr_address_translate_sys2dram(
			&ddr_cfg,
			ddr_addr,
			&rank,
			&bank,
			&bg,
			&col,
			&row);
		if (err) {
			printf("al_ddr_address_translate_sys2dram failed!\n");
			return 1;
		}

		err = al_ddr_ecc_data_poison_enable(
			&ddr_cfg,
			rank,
			bank,
			bg,
			col,
			row);
		if (err) {
			printf("al_ddr_ecc_data_poison_enable failed!\n");
			return 1;
		}

		printf("Poisoned DRAM address %08x (rank %u, bank %u, column %u, row %u)\n",
			ddr_addr, rank, bank, col, row);
	} else {
		err = al_ddr_ecc_data_poison_disable(&ddr_cfg);
		if (err) {
			printf("al_ddr_ecc_data_poison_disable failed!\n");
			return 1;
		}

		printf("Disabled poisoning\n");
	}

	return 0;
}

int do_ddr_training_results(
	cmd_tbl_t *cmdtp,
	int flag,
	int argc,
	char *const argv[])
{
	struct al_ddr_phy_training_results results;
	int octet, rank;
	int active_byte_lanes[AL_DDR_PHY_NUM_BYTE_LANES];
	struct al_ddr_cfg ddr_cfg;
	int err;

	if (argc != 1)
		return -1;

	err = al_ddr_cfg_init(
		(void __iomem *)AL_NB_SERVICE_BASE,
		(void __iomem *)AL_NB_DDR_CTL_BASE,
		(void __iomem *)AL_NB_DDR_PHY_BASE,
		&ddr_cfg);
	if (err) {
		printf("al_ddr_cfg_init failed!\n");
		return -1;
	}

	al_ddr_active_byte_lanes_get(
		&ddr_cfg,
		active_byte_lanes);

	al_ddr_phy_training_results_get(
		&ddr_cfg,
		&results);

	printf(" ___________________________________________________________________________________________\n");
	printf("|     | #Taps DLL | ---W R I T E  L E V E L I N G---   | ---R E A D   D Q S   D E L A Y---  |\n");
	printf("|OCTET| init  now | rank3   rank#2   rank#1   rank#0   | rank3   rank#2   rank#1   rank#0   |\n");
	printf("|-----+-----------+------------------------------------+------------------------------------|\n");

	for (octet = 0; octet <= 8; octet++) {
		if (!active_byte_lanes[octet])
			continue;

		printf("| #%d: |", octet);
		printf("  %3d  ", results.octets[octet].dll_num_taps_init);
		printf("%3d |", results.octets[octet].dll_num_taps_curr);

		for (rank = 3; rank >= 0; rank--) {
			printf("%3d", results.octets[octet].wld[rank]);

			switch (results.octets[octet].wld_extra[rank]) {
			case 0:
				printf("-CK ");
				break;
			case 1:
				printf("    ");
				break;
			case 2:
				printf("+CK ");
				break;
			default:
				printf("+XX ");
				break;
			}

			printf("  ");
		}

		printf("|");

		for (rank = 3; rank >= 0; rank--) {
			printf("%3d", results.octets[octet].rdqsgd[rank]);
			printf("+%dCK", results.octets[octet].rdqsgd_extra[rank]);
			printf("  ");
		}

		printf("|\n");
	}

	printf("|_____|___________|____________________________________|____________________________________|\n");

	printf(" ____________________________________________________________\n");
	printf("| OCTET | WDQD | RDQSD | RDQSND | WLMN | WLMX | RLMN | RLMX |\n");
	printf("|-----------------------------------------------------------|\n");

	for (octet = 0; octet <= 8; octet++) {
		if (!active_byte_lanes[octet])
			continue;

		printf("| #%d:   | %3d  | %3d   | %3d    | %3d  | %3d  | %3d  | %3d  |\n",
			octet,
			results.octets[octet].wdqd,
			results.octets[octet].rdqsd,
			results.octets[octet].rdqsnd,
			results.octets[octet].dtwlmn +
			results.octets[octet].dtwbmn,
			results.octets[octet].dtwlmx +
			results.octets[octet].dtwbmx,
			results.octets[octet].dtrlmn +
			results.octets[octet].dtrbmn,
			results.octets[octet].dtrlmx +
			results.octets[octet].dtrbmx);
	}

	printf("|_______|______|_______|________|______|______|______|______|\n");

	return 0;
}

U_BOOT_CMD(
	ddr_ecc_stats, 2, 0, do_ddr_ecc_stats,
	"DDR ECC statistics",
	"ddr_ecc_stats [clear - clear counters and pending interrupts]\n\n");

U_BOOT_CMD(
	ddr_ecc_poison, 2, 0, do_ddr_ecc_poison,
	"DDR ECC poisoning",
	"ddr_ecc_poison <DRAM address to poison, or 0 to cancel poisoning>\n\n");

U_BOOT_CMD(
	ddr_training_results, 1, 0, do_ddr_training_results,
	"DDR training results",
	"ddr_training_results\n\n");

