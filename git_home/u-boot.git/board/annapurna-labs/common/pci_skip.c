 /*
   * board/annapurna-labs/common/pci_skip.c
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

int pci_skip_dev(struct pci_controller *hose, pci_dev_t dev)
{
	/* Don't skip bus>0, dev==0 */
	if (PCI_BUS(dev) && !PCI_DEV(dev))
		return 0;

	/* Skip bus>0, dev>0 */
	if (PCI_BUS(dev) && PCI_DEV(dev))
		return 1;

	return 0;
}

