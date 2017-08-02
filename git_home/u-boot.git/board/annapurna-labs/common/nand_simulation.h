
#ifndef __NAND_SIMULATION_H__
#define __NAND_SIMULATION_H__

#include <common.h>
#include "nand.h"
#include "al_hal_pbs_regs.h"

struct al_nand_ctrl_obj {
	int dummy;
};

struct al_nand_ecc_config {
	int algorithm;
	int num_corr_bits;
	int messageSize;
	int spareAreaOffset;
};

struct al_nand_dev_properties {
	int timingMode;
	int pageSize;
	int num_col_cyc;
	int num_row_cyc;
};

struct al_nand_extra_dev_properties {
	unsigned int				pageSize;
	int					eccIsEnabled;
};

#define AL_NAND_CMD_SEQ_ENTRY(type, arg) (((type) << 8) | (arg))

int nand_init_simulation(
			void			*obj,
			void __iomem		*regs_base,
			void __iomem		*wrap_regs_base,
			void __iomem		*cmd_buff_base,
			void __iomem		*data_buff_base,
			void			*raid_dma,
			uint32_t		raid_dma_qid,
			char			*name);

void nand_cmd_ctrl_simulation(void *nand, uint32_t cmd);

int nand_dev_ready_simulation(void *nand);

int nand_data_buff_read_simulation(
			void			*obj,
			int			num_bytes,
			int			num_bytes_skip_head,
			int			num_bytes_skip_tail,
			uint8_t			*buff);

int nand_data_buff_write_simulation(
			void				*obj,
			int				num_bytes,
			const uint8_t			*buff);

int nand_cmd_buff_is_empty_simulation(
			void		*obj);

int nand_scan_bbt_simulate(struct mtd_info *mtd);

int nand_properties_decode_simulate(
	struct al_pbs_regs			*regs_base,
	struct al_nand_dev_properties		*dev_properties,
	struct al_nand_ecc_config		*ecc_config,
	struct al_nand_extra_dev_properties	*dev_ext_props);

/* hal replace */
#define al_nand_cmd_single_execute nand_cmd_ctrl_simulation
#define al_nand_dev_is_ready nand_dev_ready_simulation
#define al_nand_init nand_init_simulation
#define al_nand_data_buff_read nand_data_buff_read_simulation
#define al_nand_data_buff_write nand_data_buff_write_simulation
#define al_nand_cmd_buff_is_empty nand_cmd_buff_is_empty_simulation
#define al_nand_properties_decode nand_properties_decode_simulate

static inline int al_nand_dev_config(void *x, void *y, void *z) { return 0; }
static inline int al_nand_dev_config_basic(void *x) { return 0; }
static inline void al_nand_dev_select(void *x, int y) {}
static inline int al_nand_wp_set_enable(void *x, uint32_t y) { return 0; }
static inline int al_nand_tx_set_enable(void *x, uint32_t y) { return 0; }
static inline int al_nand_ecc_set_enabled(void *x, uint32_t y) { return 0; }
static inline int al_nand_config_basic(void *x) { return 0; }
static inline int al_nand_corr_err_get(void *x) { return 0; }
static inline int al_nand_uncorr_err_get(void *x) { return 0; }
static inline int al_nand_uncorr_err_clear(void *x) { return 0; }
static inline int al_nand_corr_err_clear(void *x) { return 0; }
static inline
	int al_nand_cw_config(void *x, uint32_t y, uint32_t z) { return 0; }

enum al_nand_command_type {
	AL_NAND_COMMAND_TYPE_NOP		= 0,
	AL_NAND_COMMAND_TYPE_CMD		= 2,
	AL_NAND_COMMAND_TYPE_ADDRESS		= 3,
	AL_NAND_COMMAND_TYPE_WAIT_CYCLE_COUNT	= 4,
	AL_NAND_COMMAND_TYPE_WAIT_FOR_READY	= 5,
	AL_NAND_COMMAND_TYPE_DATA_READ_COUNT	= 6,
	AL_NAND_COMMAND_TYPE_DATA_WRITE_COUNT	= 7,
	AL_NAND_COMMAND_TYPE_STATUS_READ	= 8,
	AL_NAND_COMMAND_TYPE_SPARE_READ_COUNT	= 9,
	AL_NAND_COMMAND_TYPE_SPARE_WRITE_COUNT	= 10,
	AL_NAND_COMMAND_TYPE_STATUS_WRITE	= 11,
};

enum al_nand_ecc_bch_num_corr_bits {
	AL_NAND_ECC_BCH_NUM_CORR_BITS_4		= 0,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_8		= 1,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_12	= 2,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_16	= 3,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_20	= 4,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_24	= 5,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_28	= 6,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_32	= 7,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_36	= 8,
	AL_NAND_ECC_BCH_NUM_CORR_BITS_40	= 9,
};

enum al_nand_device_timing_mode {
	AL_NAND_DEVICE_TIMING_MODE_ONFI_0	= 0,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_1	= 1,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_2	= 2,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_3	= 3,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_4	= 4,
	AL_NAND_DEVICE_TIMING_MODE_ONFI_5	= 5,
	AL_NAND_DEVICE_TIMING_MODE_MANUAL	= 6,
};

enum al_nand_ecc_algorithm {
	AL_NAND_ECC_ALGORITHM_HAMMING	= 0,
	AL_NAND_ECC_ALGORITHM_BCH	= 1,
};

enum al_nand_device_page_size {
	AL_NAND_DEVICE_PAGE_SIZE_2K	= 0,
	AL_NAND_DEVICE_PAGE_SIZE_4K	= 1,
	AL_NAND_DEVICE_PAGE_SIZE_8K	= 2,
	AL_NAND_DEVICE_PAGE_SIZE_16K	= 3,
	AL_NAND_DEVICE_PAGE_SIZE_512	= 4,
};

#endif
