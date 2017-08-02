 /*
   * drivers/spi/al_spi.c
   * Thie file contains the SPI driver for the Annapurna Labs
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
#include <spi.h>
#include <asm/io.h>
#include <errno.h>

#include "al_hal_spi.h"
#include "al_hal_iomap.h"

#define MAX_CMD_LEN			10

#define AL_SPI_TIMEOUT	4000000		/* will be defined on board config */

struct al_spi_bus {
	int			cs_index;
	unsigned int		num_refs;
	uint32_t		freq;
	enum al_spi_phase_t	phase;
	enum al_spi_polarity_t	polarity;
};

struct al_spi_slave {
	struct spi_slave slave_st;
	uint32_t freq;
	/* mode structure (according to spi.h):
	 * {LOOP, 3WIRE, LSB_FIRST, CS_HIGH, CPOL, CPHA} */
	uint8_t mode;
	uint8_t initialized;
	uint8_t last_cmd[MAX_CMD_LEN];
	uint8_t last_cmd_length;
};

static struct al_spi_interface spi_if;

static struct al_spi_slave al_spi_slaves[] = {
	{
		.slave_st.cs = 0,
		.initialized = 0,
	},
	{
		.slave_st.cs = 1,
		.initialized = 0,
	},
	{
		.slave_st.cs = 2,
		.initialized = 0,
	},
	{
		.slave_st.cs = 3,
		.initialized = 0,
	}
};

static struct al_spi_bus al_spi_bus = {
	.cs_index = -1,
};

static inline int slave_check(struct spi_slave *slave,
				char const *origin)
{
	if (slave == NULL) {
		al_err("%s: slave input is NULL\n", origin);
		return -EINVAL;
	}
	if (al_spi_slaves[slave->cs].initialized == 0) {
		al_err("%s: slave with cs=%04X is uninitialized\n",
			origin, slave->cs);
		return -EINVAL;
	}
	return 0;
}


/* Update the appropriate slave on the al_spi_slaves datastructures
 * with the provided slave arguments.
 */
struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode) {
	al_info("%s: Entered\n\t", __func__);
	al_info("bus=0x%02X | cs=0x%02X | max_hz=0x%08X | mode=0x%04X\n",
			bus, cs, max_hz, mode);

	if (spi_cs_is_valid(bus, cs) == 0) {
		al_err("%s: cs out of bound - cs=%d.", __func__, cs);
		return NULL;
	}

	al_spi_slaves[cs].slave_st.bus = bus;
	al_spi_slaves[cs].slave_st.cs = cs;
	al_spi_slaves[cs].freq = max_hz;
	al_spi_slaves[cs].mode = mode;
	al_spi_slaves[cs].initialized = 1;
	al_spi_slaves[cs].last_cmd_length = 0;

	return &al_spi_slaves[cs].slave_st;
}

void spi_print_buff(char *name, const uint8_t *buff, unsigned int len)
{
	int i, j;

	debug("%s\n", name);

	for (i = 0; i < len; i += 8) {
		debug("%08x: ", i);
		for (j = i; (j < i + 8) && (j < len); j++, buff++)
			debug("%02x ", *buff);
		debug("\n");
	}
	debug("\n");
}

/* Perform an SPI transfer.
 */
int  spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags) {
	uint32_t bytelen;
	int ret = 0;
	struct al_spi_slave *spi_slave = NULL;

	al_info("%s: Entered\n\t", __func__);
	al_info("bitlen=0x%08X | dout=%p | din=%p | flags=0x%08X\n",
		bitlen, dout, din, (uint32_t)flags);

	/* Input check & handle */
	if ((bitlen & 0x7) != 0x0) { /* bitlen isn't a multiplication of 8 */
		al_err("%s: tx length cannot be parsed as bytes. bitlen=%d\n",
			__func__, bitlen);
		return -EINVAL;
	}
	bytelen = bitlen/8;
	if (slave_check(slave, __func__))
		return -EINVAL;
	/* Cast the slave struct into al_spi_slave struct */
	spi_slave = container_of(slave, struct al_spi_slave, slave_st);

	/* Check if we are on the command phase:
	 * flags should be set to BEGIN
	 * din should be NULL, since we are not expecting incoming data */
	if ((flags == SPI_XFER_BEGIN) && !din && dout) {
		debug("%s: Command only phase\n", __func__);
		/* Command phase */
		/* validate command length */
		if (bytelen > MAX_CMD_LEN) {
			printf("bytelen > MAX_CMD_LEN\n");
			return -EINVAL;
		}

		/* copy the command to the slave data structure */
		memcpy(spi_slave->last_cmd, dout, bytelen);
		spi_slave->last_cmd_length = bytelen;

		spi_print_buff("cmd", spi_slave->last_cmd, spi_slave->last_cmd_length);
	/* Check if we are on the read data phase:
	 * flags should be set to END
	 * dout should be NULL, since we are not expecting outdoing data */
	} else if (((flags == SPI_XFER_END) || (flags == 0)) && !dout && din) {
		debug("%s: Read with previous command\n", __func__);
		/* Read data phase */

		spi_print_buff("cmd", spi_slave->last_cmd, spi_slave->last_cmd_length);

		ret = al_spi_read(&spi_if, spi_slave->last_cmd,
				spi_slave->last_cmd_length, din, bytelen,
				spi_slave->slave_st.cs, AL_SPI_TIMEOUT);

		spi_print_buff("din", din, bytelen > 32 ? 32 : bytelen);
	/* Check if we are on the write data phase:
	 * flags should be set to END
	 * dout should be NULL, since we are not expecting incoming data */
	} else if ((flags == SPI_XFER_END) && !din && dout) {
		debug("%s: Write with previous command\n", __func__);
		/* Write data phase */

		spi_print_buff("cmd", spi_slave->last_cmd, spi_slave->last_cmd_length);
		spi_print_buff("dout", dout, bytelen);

		ret = al_spi_write(&spi_if, spi_slave->last_cmd,
				spi_slave->last_cmd_length, dout, bytelen,
				spi_slave->slave_st.cs, AL_SPI_TIMEOUT);
		spi_slave->last_cmd_length = 0;
	} else if ((flags == (SPI_XFER_BEGIN | SPI_XFER_END)) && !din && dout) {
		debug("%s: Full write command\n", __func__);
		/* Write data phase */

		spi_print_buff("dout", dout, bytelen);

		ret = al_spi_write(&spi_if, NULL, 0, dout, bytelen,
				spi_slave->slave_st.cs, AL_SPI_TIMEOUT);
	/* De-assert CS */
	} else if ((flags == SPI_XFER_END) && !din && !dout && !bytelen) {
		debug("%s: De-assert CS\n", __func__);
		ret = al_spi_write(&spi_if, NULL, 0, NULL, 0, spi_slave->slave_st.cs, AL_SPI_TIMEOUT);
		spi_slave->last_cmd_length = 0;
	/* Check if we are on a single cycle command (e.g erase - no in/output):
	 * flags should be set to BEGIN and END simultaniously
	 * din and dout should be NULL, since we are not expecting any data */
	} else if ((flags == (SPI_XFER_BEGIN | SPI_XFER_END)) && !din && !dout) {
		debug("%s: Command without data\n", __func__);
		assert(spi_slave->last_cmd_length == 0);
		/* Single cycle command phase */
		memcpy(spi_slave->last_cmd, dout, bytelen);
		spi_slave->last_cmd_length = bytelen;

		spi_print_buff("cmd", spi_slave->last_cmd, spi_slave->last_cmd_length);

		ret = al_spi_write(&spi_if, spi_slave->last_cmd,
				spi_slave->last_cmd_length, NULL, 0,
				spi_slave->slave_st.cs, AL_SPI_TIMEOUT);
		spi_slave->last_cmd_length = 0;
	} else {
		al_err("%s: Invalid transfer\n", __func__);

		if (dout)
			spi_print_buff("din", dout, bytelen);

		return -EINVAL;
	}

	return ret;
}

/*
 * Mark a slave as uninitialized, to prevent future usage.
 */
void spi_free_slave(struct spi_slave *slave)
{
	al_info("%s: Entered\n", __func__);
}


int spi_claim_bus(struct spi_slave *slave)
{
	int ret = 0;
	struct al_spi_slave *spi_slave;
	uint8_t polarity, phase;

	al_info("%s: Entered\n", __func__);

	ret = slave_check(slave, __func__);
	if (ret != 0)
		return ret;

	spi_slave = container_of(slave, struct al_spi_slave, slave_st);
	phase = (spi_slave->mode & SPI_CPHA) == 0 ?
				AL_SPI_PHASE_SLAVE_SELECT :
				AL_SPI_PHASE_CLOCK;
	polarity = (spi_slave->mode & SPI_CPOL) == 0 ?
				AL_SPI_POLARITY_INACTIVE_LOW :
				AL_SPI_POLARITY_INACTIVE_HIGH;

	if ((al_spi_bus.cs_index == spi_slave->slave_st.cs) &&
		(al_spi_bus.freq == spi_slave->freq) &&
		(al_spi_bus.phase == phase) &&
		(al_spi_bus.polarity == polarity)) {
		al_info("%s: same configuration already claimed\n", __func__);
		al_spi_bus.num_refs++;
		return 0;
	}

	ret = al_spi_claim_bus(&spi_if, spi_slave->freq, phase, polarity,
				spi_slave->slave_st.cs);
	if (ret)
		al_err("%s: Failed with error#%d\n", __func__, ret);

	if (!ret) {
		al_spi_bus.cs_index = spi_slave->slave_st.cs;
		al_spi_bus.num_refs = 1;
		al_spi_bus.freq = spi_slave->freq;
		al_spi_bus.phase = phase;
		al_spi_bus.polarity = polarity;
	}

	return ret;
}

void spi_release_bus(struct spi_slave *slave)
{
	al_info("%s: Entered\n", __func__);

	/* Release slave only if input slave is valid */
	if (slave_check(slave, __func__) == 0) {
		if (al_spi_bus.num_refs == 1) {
			al_spi_release_bus(&spi_if, slave->cs);
			al_spi_bus.cs_index = -1;
		}

		if (al_spi_bus.num_refs > 0)
			al_spi_bus.num_refs--;
	}
}

/*
 * hz is the SPI device speed, measured in Hz
 */
void spi_set_speed(struct spi_slave *slave, uint hz)
{
	al_info("%s: Entered\n\t", __func__);
	al_info("hz = 0x%08X\n", hz);

	/* Set speed only if input slave is valid */
	if (slave_check(slave, __func__) == 0)
		al_spi_slaves[slave->cs].freq = hz;
}

void spi_init()
{
	al_spi_init(&spi_if, (uint32_t *)AL_SPI_MASTER_BASE, CONFIG_AL_SB_CLK_FREQ);
	al_spi_baud_rate_set(al_spi_get_baudr(&spi_if));
}


int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	al_info("%s: Entered\n", __func__);

	return (cs > AL_SPI_CS_NUM) ? 0 : -EINVAL;
}

void spi_cs_activate(struct spi_slave *slave)
{
	al_info("%s: Entered\n", __func__);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	al_info("%s: Entered\n", __func__);
}
