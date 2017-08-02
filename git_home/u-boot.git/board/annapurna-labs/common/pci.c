 /*
   * board/annapurna-labs/common/pci.c
   * Thie file contains the PCI driver for the Annapurna Labs
   * architecture.
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
#include <pci.h>
#include <asm/io.h>

#include "al_globals.h"
#include "al_init_pcie.h"
#include "al_init_pcie_debug.h"
#include "al_hal_unit_adapter_regs.h"

/* --->>> Constants definitions <<<--- */
#define LINK_UP_WAIT			100	/*in ms*/

#define CONFIG_PCIE_0_MEMORY_BUS	AL_PCIE_0_BASE
#define CONFIG_PCIE_0_MEMORY_PHYS	AL_PCIE_0_BASE
#define CONFIG_PCIE_0_MEMORY_SIZE	AL_PCIE_0_SIZE
#define CONFIG_PCIE_0_ECAM_BUS		AL_PCIE_0_ECAM_BASE
#define CONFIG_PCIE_0_ECAM_PHYS		AL_PCIE_0_ECAM_BASE
#define CONFIG_PCIE_0_ECAM_SIZE		AL_PCIE_0_ECAM_SIZE

#define CONFIG_PCIE_1_MEMORY_BUS	AL_PCIE_1_BASE
#define CONFIG_PCIE_1_MEMORY_PHYS	AL_PCIE_1_BASE
#define CONFIG_PCIE_1_MEMORY_SIZE	AL_PCIE_1_SIZE
#define CONFIG_PCIE_1_ECAM_BUS		AL_PCIE_1_ECAM_BASE
#define CONFIG_PCIE_1_ECAM_PHYS		AL_PCIE_1_ECAM_BASE
#define CONFIG_PCIE_1_ECAM_SIZE		AL_PCIE_1_ECAM_SIZE

#define CONFIG_PCIE_2_MEMORY_BUS	AL_PCIE_2_BASE
#define CONFIG_PCIE_2_MEMORY_PHYS	AL_PCIE_2_BASE
#define CONFIG_PCIE_2_MEMORY_SIZE	AL_PCIE_2_SIZE
#define CONFIG_PCIE_2_ECAM_BUS		AL_PCIE_2_ECAM_BASE
#define CONFIG_PCIE_2_ECAM_PHYS		AL_PCIE_2_ECAM_BASE
#define CONFIG_PCIE_2_ECAM_SIZE		AL_PCIE_2_ECAM_SIZE

#define CONFIG_PCIE_INT_MEMORY_BUS	AL_PCIE_INT_BASE
#define CONFIG_PCIE_INT_MEMORY_PHYS	AL_PCIE_INT_BASE
#define CONFIG_PCIE_INT_MEMORY_SIZE	AL_PCIE_INT_SIZE
#define CONFIG_PCIE_INT_ECAM_BUS	AL_PCIE_INT_ECAM_BASE
#define CONFIG_PCIE_INT_ECAM_PHYS	AL_PCIE_INT_ECAM_BASE
#define CONFIG_PCIE_INT_ECAM_SIZE	AL_PCIE_INT_ECAM_SIZE

#define MAX_NUM_DEVICES			20

/* --->>> MACROS <<<--- */
#define AL_PCI_CONFIG_REG_TO_ADDR(Bus, Dev, Func, Reg)	\
		(((Bus) << 20) | ((Dev) << 15) | ((Func) << 12) | (Reg))

/* --->>> Data Structures <<<--- */

struct configured_device {
	struct pci_controller	*hose;
	pci_dev_t		dev;
};

/* The ecam address struct is being used for parsing the ecam address parameter
   out of the hose private_data field (see read/write_config functions) */
struct al_ecam_addr {
	uint32_t ecam_addr;
};

/* The following struct is being used to initialize the pci/e devices */
struct al_pcie_init_struct {
	unsigned int			idx;
	struct pci_controller		*hose;
	struct al_ecam_addr		*ecam;
	pci_addr_t			bus_start;
	phys_addr_t			phys_start;
	pci_size_t			size;
	unsigned long			flags;
	struct al_init_pcie_handle	pcie_handle;
};

/* --->>> Static parameters <<<--- */
static struct configured_device configured_devices[MAX_NUM_DEVICES];
static unsigned int num_configured_devices;

static void al_pcie_config_device(struct pci_controller *hose,
				pci_dev_t dev,
				struct pci_config_table *entry);


static struct pci_controller	al_pcie_int_hose;
static struct al_ecam_addr	al_int_ecam_addr = {
	.ecam_addr	= CONFIG_PCIE_INT_ECAM_PHYS
};

#ifdef CONFIG_AL_PCIE_0
static struct pci_controller	al_pcie_0_hose;
static struct al_ecam_addr	al_dev0_ecam_addr = {
	.ecam_addr	= CONFIG_PCIE_0_ECAM_PHYS
};
#endif
#ifdef CONFIG_AL_PCIE_1
static struct pci_controller	al_pcie_1_hose;
static struct al_ecam_addr	al_dev1_ecam_addr = {
	.ecam_addr	= CONFIG_PCIE_1_ECAM_PHYS
};
#endif
#ifdef CONFIG_AL_PCIE_2
static struct pci_controller	al_pcie_2_hose;
static struct al_ecam_addr	al_dev2_ecam_addr = {
	.ecam_addr	= CONFIG_PCIE_2_ECAM_PHYS
};
#endif

/* Main data structure for external PCIE devices */
static struct al_pcie_init_struct init_struct_array[] = {
#ifdef CONFIG_AL_PCIE_0
	{
		.idx			= 0,
		.hose			= &al_pcie_0_hose,
		.ecam			= &al_dev0_ecam_addr,
		.bus_start		= CONFIG_PCIE_0_MEMORY_BUS,
		.phys_start		= CONFIG_PCIE_0_MEMORY_PHYS,
		.size			= CONFIG_PCIE_0_MEMORY_SIZE,
		.flags			= PCI_REGION_MEM,
	},
#endif
#ifdef CONFIG_AL_PCIE_1
	{
		.idx			= 1,
		.hose			= &al_pcie_1_hose,
		.ecam			= &al_dev1_ecam_addr,
		.bus_start		= CONFIG_PCIE_1_MEMORY_BUS,
		.phys_start		= CONFIG_PCIE_1_MEMORY_PHYS,
		.size			= CONFIG_PCIE_1_MEMORY_SIZE,
		.flags			= PCI_REGION_MEM,
	},
#endif
#ifdef CONFIG_AL_PCIE_2
	{
		.idx			= 2,
		.hose			= &al_pcie_2_hose,
		.ecam			= &al_dev2_ecam_addr,
		.bus_start		= CONFIG_PCIE_2_MEMORY_BUS,
		.phys_start		= CONFIG_PCIE_2_MEMORY_PHYS,
		.size			= CONFIG_PCIE_2_MEMORY_SIZE,
		.flags			= PCI_REGION_MEM,
	},
#endif
	{ 0 }
};


/* Define memory allocation for PCIe units */
static struct pci_config_table al_pci_config_table[] = {
	{
		.vendor		= PCI_ANY_ID,
		.device		= PCI_ANY_ID,
		.class		= PCI_ANY_ID,
		.bus		= PCI_ANY_ID,
		.dev		= PCI_ANY_ID,
		.func		= PCI_ANY_ID,
		.config_device	= al_pcie_config_device,
	}
};

/* --->>> Static functions <<<--- */

/* The following is very similar to U-Boot's pci_hose_config_device function,
 * except it passes mem variable by pointer instead of by value,
 * so the outer datastructures can be updated according to the function's
 * operations
 */
static int al_pci_hose_config_device(struct pci_controller *hose,
			   pci_dev_t dev,
			   unsigned long io,
			   pci_addr_t *mem,
			   unsigned long command)
{
	unsigned int bar_response, old_command;
	pci_addr_t bar_value;
	pci_size_t bar_size;
	unsigned char pin;
	int bar, found_mem64;

	debug ("PCI Config: I/O=0x%lx, Memory=0x%llx, Command=0x%lx\n",
		io, (u64)*mem, command);

	pci_hose_write_config_dword (hose, dev, PCI_COMMAND, 0);

	for (bar = PCI_BASE_ADDRESS_0; bar <= PCI_BASE_ADDRESS_5; bar += 4) {
		pci_hose_write_config_dword (hose, dev, bar, 0xffffffff);
		pci_hose_read_config_dword (hose, dev, bar, &bar_response);

		if (!bar_response)
			continue;

		found_mem64 = 0;

		/* Check the BAR type and set our address mask */
		if (bar_response & PCI_BASE_ADDRESS_SPACE) {
			bar_size = ~(bar_response & PCI_BASE_ADDRESS_IO_MASK) + 1;
			/* round up region base address to a multiple of size */
			io = ((io - 1) | (bar_size - 1)) + 1;
			bar_value = io;
			/* compute new region base address */
			io = io + bar_size;
		} else {
			if ((bar_response & PCI_BASE_ADDRESS_MEM_TYPE_MASK) ==
				PCI_BASE_ADDRESS_MEM_TYPE_64) {
				bar_size = (u32)(~(bar_response & PCI_BASE_ADDRESS_MEM_MASK) + 1);
				found_mem64 = 1;
			} else {
				bar_size = (u32)(~(bar_response & PCI_BASE_ADDRESS_MEM_MASK) + 1);
			}

			/* round up region base address to multiple of size */
			*mem = ((*mem - 1) | (bar_size - 1)) + 1;
			bar_value = *mem;
			/* compute new region base address */
			*mem = *mem + bar_size;
		}

		/* Write it out and update our limit */
		pci_hose_write_config_dword (hose, dev, bar, (u32)bar_value);

		if (found_mem64) {
			bar += 4;
#ifdef CONFIG_SYS_PCI_64BIT
			pci_hose_write_config_dword(hose, dev, bar, (u32)(bar_value>>32));
#else
			pci_hose_write_config_dword (hose, dev, bar, 0x00000000);
#endif
		}
	}

	/* Configure Cache Line Size Register */
	pci_hose_write_config_byte (hose, dev, PCI_CACHE_LINE_SIZE, 0x08);

	/* Configure Latency Timer */
	pci_hose_write_config_byte (hose, dev, PCI_LATENCY_TIMER, 0x80);

	/* Disable interrupt line, if device says it wants to use interrupts */
	pci_hose_read_config_byte (hose, dev, PCI_INTERRUPT_PIN, &pin);
	if (pin != 0) {
		pci_hose_write_config_byte (hose, dev, PCI_INTERRUPT_LINE, 0xff);
	}

	pci_hose_read_config_dword (hose, dev, PCI_COMMAND, &old_command);
	pci_hose_write_config_dword (hose, dev, PCI_COMMAND,
				     (old_command & 0xffff0000) | command);

	return 0;
}

/* The pci config function expects hose->regions[0] to
 * be configured as the pci region.
 */
static void al_pcie_config_device(struct pci_controller *hose,
				pci_dev_t dev,
				struct pci_config_table *entry)
{
	pci_addr_t *pcie_device_start =
			(pci_addr_t *)(&hose->regions[0].bus_lower);

	al_pci_hose_config_device(hose, dev,
				0,			/* IO Address*/
				pcie_device_start,	/* MEM Address */
				entry->priv[2]);	/* Command*/

	if (num_configured_devices < MAX_NUM_DEVICES) {
		configured_devices[num_configured_devices].hose = hose;
		configured_devices[num_configured_devices].dev = dev;
		num_configured_devices++;
	} else {
		printf("%s: exceeded maximal number of devices!\n", __func__);
	}
}

void pci_cleanup(void)
{
	unsigned int i;

	/* Perform FLR to external devices */
	for (i = 0; i < num_configured_devices; i++) {
		struct pci_controller *hose = configured_devices[i].hose;
		pci_dev_t dev = configured_devices[i].dev;
		int pcie_cap_off;
		uint32_t pcie_dev_cap;
		uint16_t pcie_dev_ctl;

		debug("PCI Cleanup: Found Bus %d, Device %d, Function %d\n",
			PCI_BUS(dev), PCI_DEV(dev), PCI_FUNC(dev));

		if (!PCI_BUS(dev))
			continue;

		pcie_cap_off = pci_hose_find_capability(hose, dev, PCI_CAP_ID_EXP);
		if (!pcie_cap_off)
			continue;

		pci_hose_read_config_dword(
			hose, dev, pcie_cap_off + AL_PCI_EXP_DEVCAP, &pcie_dev_cap);
		if (!(pcie_dev_cap & AL_PCI_EXP_DEVCAP_FLR))
			continue;

		printf("PCI Cleanup: FLR %d.%d.%d\n", PCI_BUS(dev), PCI_DEV(dev), PCI_FUNC(dev));
		pci_hose_read_config_word(
			hose, dev, pcie_cap_off + AL_PCI_EXP_DEVCTL, &pcie_dev_ctl);
		pcie_dev_ctl |= AL_PCI_EXP_DEVCTL_BCR_FLR;
		pci_hose_write_config_word(
			hose, dev, pcie_cap_off + AL_PCI_EXP_DEVCTL, pcie_dev_ctl);
	}
}

/* The following function maps a given logical bus number into a physical bus
 * number.
 * Our current architecture supports only 1 bus per device. Bus number 0 is
 * reserved to the internal PCI.
 */
static int al_pcie_bus_map(
		struct pci_controller	*hose,
		int			bus_in,
		int			*bus_out)
{
	if (bus_in == 0)
		*bus_out = 0;
	else if ((bus_in - hose->first_busno) == 0)
		*bus_out = 0;
	else {
		al_err("%s: Error: single bus per controller is supported.\n"
				"recevied bus: %d, controller first bus: %d\n",
				__func__, bus_in, hose->first_busno);
		return -EINVAL;
	}

	return 0;
}

extern int abort_is_pending(void);
extern void soak_all_aborts(void);
static unsigned int waiting_for_device;

static noinline unsigned int readl_might_abort(uintptr_t reg_addr, uint32_t *val)
{
	unsigned int aborted;

	*val = readl(reg_addr);
	mb();
	aborted = abort_is_pending();
	if (aborted)
		soak_all_aborts();

	return aborted;
}

/* The following are read/write functions for the PCI config space.
 * These functions are being used as a base for 8/16 bit commands
 * as well, using U-Boot's coversion functions.
 */
static int al_pci_read_config32(struct pci_controller *hose, pci_dev_t dev,
			int offset, uint32_t *val)
{
	int bus, device, function;
	pci_dev_t anpa_dev;
	uint32_t ecam_addr;
	int err = 0;

	/* Hide bus>0, dev>0 */
	if (PCI_BUS(dev) && PCI_DEV(dev)) {
		*val = 0;
		return 0;
	}

	/* Parse the pci_dev_t variable */
	err = al_pcie_bus_map(hose, PCI_BUS(dev), &bus);
	if (err)
		return err;
	device = PCI_DEV(dev);
	function = PCI_FUNC(dev);
	/* Get the ECAM address out of the hose */
	ecam_addr = ((struct al_ecam_addr *) hose->priv_data)->ecam_addr;
	/* Reconfigure the device according to our specification */
	anpa_dev = AL_PCI_CONFIG_REG_TO_ADDR(bus, device, function, offset);

	if ((!PCI_BUS(dev)) || function) {
		*val = readl((uintptr_t)(ecam_addr | anpa_dev));
	} else {
		while (1) {
			if (readl_might_abort((uintptr_t)(ecam_addr | anpa_dev), val)) {
				if (!waiting_for_device)
					printf("PCIe device is responding with CRS, waiting... ");
				waiting_for_device = 1;
			} else {
				if (waiting_for_device)
					printf("Done\n");
				waiting_for_device = 0;
				break;
			}
		}
	}

	debug("%s(%d.%d.%d, %08x) --> %08x\n", __func__, bus, device, function, offset, *val);

	return 0;
}

static int al_pci_write_config32(struct pci_controller *hose, pci_dev_t dev,
			int offset, uint32_t val)
{
	int bus, device, function;
	pci_dev_t anpa_dev;
	uint32_t ecam_addr;
	int err = 0;

	/* Hide bus>0, dev>0 */
	if (PCI_BUS(dev) && PCI_DEV(dev))
		return 0;

	/* Parse the pci_dev_t variable */
	err = al_pcie_bus_map(hose, PCI_BUS(dev), &bus);
	if (err)
		return err;
	device = PCI_DEV(dev);
	function = PCI_FUNC(dev);
	/* Get the ECAM address out of the hose */
	ecam_addr = ((struct al_ecam_addr *) hose->priv_data)->ecam_addr;
	/* Reconfigure the device according to our specification */
	anpa_dev = AL_PCI_CONFIG_REG_TO_ADDR(bus, device, function, offset);

	debug("%s(%d.%d.%d, %08x) <-- %08x\n", __func__, bus, device, function, offset, val);

	writel(val, (uintptr_t)(ecam_addr | anpa_dev));

	return 0;
}

/* Initializes the given pci controller (hose)
 *
 * Returns the next available bus number
 */
static void al_pci_init_hose(
		struct pci_controller	*hose,
		struct al_ecam_addr	*ecam,
		pci_addr_t		bus_start,
		phys_addr_t		phys_start,
		pci_size_t		size,
		unsigned long		flags,
		unsigned int		first_busno)
{
	hose->config_table = al_pci_config_table;

	hose->first_busno = first_busno;
	hose->last_busno = 0xFF;

	/* Memory space */
	pci_set_region(hose->regions + 0,
		bus_start,
		phys_start,
		size,
		flags);

	hose->regions[0].bus_lower = phys_start;

	hose->region_count = 1;

	/* Define the read/write config address functions */
	pci_set_ops(hose,
		pci_hose_read_config_byte_via_dword,
		pci_hose_read_config_word_via_dword,
		al_pci_read_config32,
		pci_hose_write_config_byte_via_dword,
		pci_hose_write_config_word_via_dword,
		al_pci_write_config32);

	hose->priv_data = ecam;
	debug("Registered PCIe controller. ECAM address:%0x\n",
			ecam->ecam_addr);

	/* Register the hose in the generic pci database */
	pci_register_hose(hose);
}


/* --->>> API functions <<<--- */

static unsigned int first_busno = 0;
static unsigned int last_busno = 0;

void pci_init_board_external(void)
{
	int err = 0, i, j;

	/* Allow external devices time to wake-up */
	udelay(LINK_UP_WAIT * 1000);

	/* Initialize the PCIE external devices according to configuration */
	for (i = 0 ; i < (ARRAY_SIZE(init_struct_array) - 1) ; ++i) {
		struct al_init_pcie_params init_pcie_params;
		struct al_init_pcie_handle *pcie_handle;
		struct al_pcie_port *pcie_port;
		struct al_pcie_link_status link_status;
		int idx;

		BUG_ON(init_struct_array[i].idx > AL_SB_PCIE_NUM);

		pcie_handle = &init_struct_array[i].pcie_handle;
		idx = init_struct_array[i].idx;

		if (!al_globals.pcie_cfg[idx].present)
			continue;

		if (al_globals.pcie_cfg[idx].ep)
			continue;

port_retry:
		init_pcie_params = default_init_pcie_params;
		init_pcie_params.port_id = idx;
		init_pcie_params.mode = AL_PCIE_OPERATING_MODE_RC;
		init_pcie_params.port_params->link_params->max_speed =
			al_globals.pcie_cfg[idx].max_speed;
		init_pcie_params.max_lanes =
			al_globals.pcie_cfg[idx].num_lanes;
		init_pcie_params.wait_for_link_timeout_ms = LINK_UP_WAIT;
		init_pcie_params.wait_for_link_silent_fail = AL_TRUE;
		for (j = 0; j < AL_MAX_NUM_OF_PFS; j++)
			init_pcie_params.pf_params[j] = NULL;

		al_init_pcie_print_params(&init_pcie_params);
		err = al_init_pcie(pcie_handle, &init_pcie_params, &link_status);

		/*
		 * if no link, try less lanes
		 * if single lane, skip this port
		 */
		if (err) {
			if (al_globals.pcie_cfg[idx].num_lanes > 1) {
				al_globals.pcie_cfg[idx].num_lanes /= 2;
				goto port_retry;
			} else if (al_globals.pcie_cfg[idx].max_speed != AL_PCIE_LINK_SPEED_GEN1) {
				al_globals.pcie_cfg[idx].max_speed = AL_PCIE_LINK_SPEED_GEN1;
				goto port_retry;
			}

			printf("%s: PCIE_%d no link found\n",
					__func__,
					idx);
			goto port_disable;
		}

		/* print PCIe link width and speed */
		printf("PCIE_%d: Link up. Speed %s Width x%d\n", idx,
				link_status.speed == AL_PCIE_LINK_SPEED_GEN1 ? "2.5GT/s" :
				link_status.speed == AL_PCIE_LINK_SPEED_GEN2 ? "5GT/s" :
				link_status.speed == AL_PCIE_LINK_SPEED_GEN3 ? "8GT/s" : "Unknown",
				link_status.lanes);

		al_globals.pcie_cfg[init_struct_array[i].idx].present = 1;
		al_globals.pcie_any_link_up = AL_TRUE;

		pcie_port = &pcie_handle->pcie_port;

		/* support only one bus (#1) */
		al_pcie_target_bus_set(pcie_port,
			   0 /*target_bus*/,
			   0xfe /*mask_target_bus*/);

		al_pci_init_hose(init_struct_array[i].hose,
				 init_struct_array[i].ecam,
				 init_struct_array[i].bus_start,
				 init_struct_array[i].phys_start,
				 init_struct_array[i].size,
				 init_struct_array[i].flags,
				 first_busno);
		last_busno = pci_hose_scan(init_struct_array[i].hose);
		init_struct_array[i].hose->last_busno = last_busno;
		first_busno = last_busno + 1;

		continue;

port_disable:
		al_globals.pcie_cfg[init_struct_array[i].idx].present = 0;
	}
}

void pci_init_board()
{
	debug("pci_init_board: Entered...\n");

	/* Initialize the internal PCI device's data structure */
	al_pci_init_hose(&al_pcie_int_hose,
					&al_int_ecam_addr,
					CONFIG_PCIE_INT_MEMORY_BUS,
					CONFIG_PCIE_INT_MEMORY_PHYS,
					CONFIG_PCIE_INT_MEMORY_SIZE,
					PCI_REGION_MEM,
					first_busno);

	last_busno = pci_hose_scan(&al_pcie_int_hose);
	al_pcie_int_hose.last_busno = last_busno;
	first_busno = last_busno + 1;

#ifndef CONFIG_CMD_AL_PCI_EXT
	pci_init_board_external();
#endif

	debug("pci_init_board: Finished\n");
}

/* Avoid skipping printing of dev 0, func 0 */
int pci_print_dev(struct pci_controller *hose, pci_dev_t dev)
{
	return 1;
}

