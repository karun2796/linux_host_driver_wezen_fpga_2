#ifdef HACK_SERDES_CONFIG
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "driver_linux.h"
#include "pal.h"

#define NRF_WIFI_RENOIR_RM_REG_0	0x00
#define RM_base_offset		0x10000

void vcu118_int_enable(struct pci_dev *pdev)
{
	unsigned long base_addr;
	unsigned char *mem_addr;
	unsigned int len;

	base_addr = pci_resource_start(pdev, PCIE_BAR_OFFSET_PCIE_EP_INT);
	len = pci_resource_len(pdev, PCIE_BAR_OFFSET_PCIE_EP_INT);

	mem_addr = ioremap_nocache(base_addr, len);

	mem_addr += PCIE_BAR_IRQ_BLOCK_OFFSET;
	mem_addr += PCIE_BAR_IRQ_BLOCK_USER_INT_ENAB_MASK_OFFSET;

	*(unsigned int *)mem_addr = 1;
}

void vcu118_serdes_reset(struct pci_dev *pdev)
{
	unsigned long base_addr;
	unsigned char *mem_addr;
	unsigned int len;

	base_addr = pci_resource_start(pdev, PCIE_BAR_OFFSET_BAR2);
	len = pci_resource_len(pdev, PCIE_BAR_OFFSET_BAR2);

	mem_addr = ioremap_nocache(base_addr, len);

	mem_addr += NRF_WIFI_RENOIR_RM_REG_0;
	mem_addr += RM_base_offset;

	*(unsigned int *)mem_addr = 0x00000a42;
}
#endif
