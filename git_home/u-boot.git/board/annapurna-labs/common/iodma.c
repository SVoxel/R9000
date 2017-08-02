/*
   * board/annapurna-labs/common/iodma.c
   *
   * Thie file contains a RAID and memory services accelerator services
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

#include "al_hal_ssm_raid.h"
#include "al_hal_unit_adapter_regs.h"

#define MEMSET_MAX_SIZE		(SZ_64K - 64)

#define RAID_UDMA_BAR PCI_BASE_ADDRESS_0
#define RAID_APP_BAR PCI_BASE_ADDRESS_4
#define DMA_Q_ID 0

#define COMPLETION_DESC_SIZE	16
#define DESCRIPTORS_PER_QUEUE	256

#define DESCRIPTORS_ALIGNMENT	16

#define TIMEOUT_MS		5000

static pci_dev_t		devno;
static uint8_t			rev_id;
static struct al_ssm_dma	raid;
static union al_udma_desc	*tx_sdesc;
static union al_udma_desc	*rx_sdesc;
static uint8_t			*rx_cdesc;

static int iodma_pci_init(void **udma_base, void **app_base)
{
	uint16_t word;

	debug("%s()\n", __func__);

	/* Find PCI device(s) */
	devno = pci_find_device(PCI_VENDOR_ID_ANNAPURNALABS,
				PCI_DEVICE_ID_AL_RAID, 0);
	if (devno < 0) {
		printf(
			"%s: pci_find_devices found no more devices\n",
			__func__);
		return -1;
	}

	debug("%s: pci_find_devices found %d:%d:%d\n",
		__func__,
		PCI_BUS(devno),
		PCI_DEV(devno),
		PCI_FUNC(devno));

	/* Read out device ID and revision ID */
	pci_read_config_byte(devno, PCI_REVISION_ID, &rev_id);

	/* Read out UDMA_BAR and APP_BAR */
	*udma_base = (void *)pci_map_bar(devno, RAID_UDMA_BAR, PCI_REGION_MEM);
	*app_base = (void *)pci_map_bar(devno, RAID_APP_BAR , PCI_REGION_MEM);

	/* Enable Bus Mastering and memory region */
	pci_write_config_word(devno, PCI_COMMAND,
			PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

	/* Check if mem accesses and Bus Mastering are enabled. */
	pci_read_config_word(devno, PCI_COMMAND, &word);
	if (!(word & PCI_COMMAND_MEMORY) ||
			(!(word & PCI_COMMAND_MASTER))) {
		debug(
			"%s: Failed enabling mem access or bus "
			"mastering!\n",
			__func__);

		debug("%s: PCI command: %04x\n", __func__, word);

		return -1;
	}

	return 0;
}

int iodma_init(void)
{
	struct al_ssm_dma_params raid_udma0_params;
	struct al_udma_q_params raid_tx_params;
	struct al_udma_q_params raid_rx_params;
	void *udma_base = NULL;
	void *app_base = NULL;
	uint32_t err;

	err = iodma_pci_init(&udma_base, &app_base);
	if (err) {
		printf("Failed to get BARs\n\n");
		return err;
	}

	tx_sdesc = (union al_udma_desc *)memalign(DESCRIPTORS_ALIGNMENT,
			(DESCRIPTORS_PER_QUEUE * sizeof(union al_udma_desc)));
	rx_sdesc = (union al_udma_desc *)memalign(DESCRIPTORS_ALIGNMENT,
			(DESCRIPTORS_PER_QUEUE * sizeof(union al_udma_desc)));
	rx_cdesc = (uint8_t *)memalign(DESCRIPTORS_ALIGNMENT,
				(DESCRIPTORS_PER_QUEUE * COMPLETION_DESC_SIZE));

	if ((tx_sdesc == NULL) || (rx_cdesc == NULL) || (rx_sdesc == NULL)) {
		printf("Failed to allocate descriptors");
		return -1;
	}

	memset(rx_cdesc, 0, DESCRIPTORS_PER_QUEUE * COMPLETION_DESC_SIZE);

	raid_udma0_params.rev_id = rev_id;
	raid_udma0_params.udma_regs_base = udma_base;
	raid_udma0_params.name = "RAID_UDMA";
	raid_udma0_params.num_of_queues = 4;

	raid_tx_params.size = DESCRIPTORS_PER_QUEUE;
	raid_tx_params.desc_base = tx_sdesc;
	raid_tx_params.desc_phy_base = (uintptr_t)tx_sdesc;
	raid_tx_params.cdesc_base = NULL;
	raid_tx_params.cdesc_phy_base = 0;
	raid_tx_params.cdesc_size = COMPLETION_DESC_SIZE;

	raid_rx_params.size = DESCRIPTORS_PER_QUEUE;
	raid_rx_params.desc_base = rx_sdesc;
	raid_rx_params.desc_phy_base = (uintptr_t)rx_sdesc;
	raid_rx_params.cdesc_base = rx_cdesc;
	raid_rx_params.cdesc_phy_base = (uintptr_t)rx_cdesc;
	raid_rx_params.cdesc_size = COMPLETION_DESC_SIZE;

	al_ssm_dma_init(&raid, &raid_udma0_params);
	al_ssm_dma_q_init(&raid, DMA_Q_ID,
				&raid_tx_params, &raid_rx_params, AL_RAID_Q);
	al_ssm_dma_state_set(&raid, UDMA_NORMAL);

	return 0;
}

void iodma_terminate(void)
{
	uint32_t val;
	uint32_t cfg_reg_store[6];
	int i;

	/* save pci registers that are reset due to FLR */
	i = 0;
	pci_read_config_dword(devno, AL_PCI_COMMAND, &cfg_reg_store[i++]);
	pci_read_config_dword(devno, 0xC, &cfg_reg_store[i++]);
	pci_read_config_dword(devno, 0x10, &cfg_reg_store[i++]);
	pci_read_config_dword(devno, 0x18, &cfg_reg_store[i++]);
	pci_read_config_dword(devno, 0x20, &cfg_reg_store[i++]);
	pci_read_config_dword(devno, 0x110, &cfg_reg_store[i++]);

	/* Function level reset */
	pci_read_config_dword(
		devno, AL_PCI_EXP_CAP_BASE + AL_PCI_EXP_DEVCTL,
		&val);
	pci_write_config_dword(
		devno, AL_PCI_EXP_CAP_BASE + AL_PCI_EXP_DEVCTL,
		val | AL_PCI_EXP_DEVCTL_BCR_FLR);

	udelay(1000);

	/* restore pci registers that are reset due to FLR */
	i = 0;
	pci_write_config_dword(devno, AL_PCI_COMMAND, cfg_reg_store[i++]);
	pci_write_config_dword(devno, 0xC, cfg_reg_store[i++]);
	pci_write_config_dword(devno, 0x10, cfg_reg_store[i++]);
	pci_write_config_dword(devno, 0x18, cfg_reg_store[i++]);
	pci_write_config_dword(devno, 0x20, cfg_reg_store[i++]);
	pci_write_config_dword(devno, 0x110, cfg_reg_store[i++]);

	free(tx_sdesc);
	free(rx_sdesc);
	free(rx_cdesc);
}

int iodma_memcpy(
	al_phys_addr_t	dst,
	al_phys_addr_t	src,
	uint64_t	size)
{
	uint32_t status;
	uint32_t err;
	unsigned int pending = 0;
	unsigned int timeout = 0;
	struct al_buf raid_src_buf;
	struct al_block raid_src_block;
	struct al_buf raid_dst_buf;
	struct al_block raid_dst_block;
	struct al_raid_transaction transaction;

	memset(&transaction, 0, sizeof(transaction));

	raid_src_block.bufs = &raid_src_buf;
	raid_src_block.num = 1;
	raid_dst_block.bufs = &raid_dst_buf;
	raid_dst_block.num = 1;

	transaction.op = AL_RAID_OP_MEM_CPY;
	transaction.flags = 0;
	transaction.srcs_blocks = &raid_src_block;
	transaction.num_of_srcs = 1;
	transaction.total_src_bufs = 1;
	transaction.dsts_blocks = &raid_dst_block;
	transaction.num_of_dsts = 1;
	transaction.total_dst_bufs = 1;

	while (size || pending) {
		uint32_t size_current = (size <= MEMSET_MAX_SIZE) ?
			(uint32_t)size : MEMSET_MAX_SIZE;
		uint32_t tx_descs_count_curr;

		if (size && (pending < (DESCRIPTORS_PER_QUEUE - 1))) {
			size -= size_current;

			raid_src_buf.addr = src;
			raid_src_buf.len = size_current;
			raid_dst_buf.addr = dst;
			raid_dst_buf.len = size_current;

			err = al_raid_dma_prepare(&raid, DMA_Q_ID, &transaction);
			if (err) {
				printf("Falied to prepare raid\n");
				return -1;
			}

			tx_descs_count_curr = transaction.tx_descs_count;

			debug("al_raid_dma_action(%u)\n", tx_descs_count_curr);
			err = al_raid_dma_action(&raid, DMA_Q_ID, tx_descs_count_curr);
			if (err) {
				printf("raid_dma_action failed\n");
				return -1;
			}

			src += size_current;
			dst += size_current;
			pending++;
		}

		if (pending) {
			if (al_raid_dma_completion(&raid, DMA_Q_ID, &status)) {
				if (status != 0)
					printf("Completion status: %u\n", status);
				else
					debug("Completion status: %u\n", status);

				pending--;
			}
		}

		if (!size) {
			udelay(1000);
			timeout++;
			if (timeout == TIMEOUT_MS) {
				printf("Timed out!\n");
				return -1;
			}
		}
	}

	return 0;
}

int iodma_memset(
	al_phys_addr_t	dst,
	uint8_t		val,
	uint64_t	size)
{
	uint32_t status;
	uint32_t err;
	unsigned int pending = 0;
	unsigned int timeout = 0;
	struct al_buf raid_dst_buf;
	struct al_block raid_dst_block;
	struct al_raid_transaction transaction;

	memset(&transaction, 0, sizeof(transaction));

	raid_dst_block.bufs = &raid_dst_buf;
	raid_dst_block.num = 1;

	transaction.op = AL_RAID_OP_MEM_SET;
	transaction.flags = 0;
	transaction.srcs_blocks = NULL;
	transaction.num_of_srcs = 0;
	transaction.total_src_bufs = 0;
	transaction.dsts_blocks = &raid_dst_block;
	transaction.num_of_dsts = 1;
	transaction.total_dst_bufs = 1;

	memset(transaction.data, val, sizeof(transaction.data));

	while (size || pending) {
		uint32_t size_current = (size <= MEMSET_MAX_SIZE) ?
			(uint32_t)size : MEMSET_MAX_SIZE;
		uint32_t tx_descs_count_curr;

		if (size && (pending < (DESCRIPTORS_PER_QUEUE - 1))) {
			size -= size_current;

			debug("iodma_memset_prepare(%08x%08x, %08x)\n",
				(uint32_t)(dst >> 32), (uint32_t)(dst & 0xffffffff),
				size_current);

			raid_dst_buf.addr = dst;
			raid_dst_buf.len = size_current;

			err = al_raid_dma_prepare(&raid, DMA_Q_ID, &transaction);
			if (err) {
				printf("Falied to prepare raid\n");
				return -1;
			}

			tx_descs_count_curr = transaction.tx_descs_count;

			debug("al_raid_dma_action(%u)\n", tx_descs_count_curr);
			err = al_raid_dma_action(&raid, DMA_Q_ID, tx_descs_count_curr);
			if (err) {
				printf("raid_dma_action failed\n");
				return -1;
			}

			dst += size_current;
			pending++;
		}

		if (pending) {
			if (al_raid_dma_completion(&raid, DMA_Q_ID, &status)) {
				if (status != 0)
					printf("Completion status: %u\n", status);
				else
					debug("Completion status: %u\n", status);

				pending--;
			}
		}

		if (!size) {
			udelay(1000);
			timeout++;
			if (timeout == TIMEOUT_MS) {
				printf("Timed out!\n");
				return -1;
			}
		}
	}

	return 0;
}

