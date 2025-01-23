/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LNX_SHIM_H__
#define __LNX_SHIM_H__

/**
 * struct lnx_shim_bus_pcie_priv - Structure to hold context information for the Linux
 * 				   specific PCIe driver context.
 * @opriv: Pointer to OSAL context.
 * @pcie_callbk_data: Callback data to be passed to the PCIe callback functions.
 * @pcie_prb_callbk: The callback function to be called when a PCIe device
 *                   has been probed.
 * @pcie_rem_callbk: The callback function to be called when a PCIe device
 *                   has been removed.
 *
 * This structure maintains the context information necessary for the operation
 * of the Linux shim. Some of the elements of the structure need to be
 * initialized during the initialization of the Linux shim while others need to
 * be kept updated over the duration of the Linux shim operation.
 */
struct lnx_shim_bus_pcie_priv {
	struct work_struct drv_reg;	
	struct nrf_wifi_osal_priv *opriv;

	struct pci_driver *pcie_drv;
	const struct pci_device_id *pdev_id;
	struct pci_dev *pdev;
};


/**
 * struct lnx_shim_bus_pcie_dev_ctx - Structure to hold context information for the Linux
 * 				      specific PCIe device context.
 * @pcie_priv: Pointer to Linux specific PCIe driver context.
 *
 */
struct lnx_shim_bus_pcie_dev_ctx {
	struct lnx_shim_bus_pcie_priv *lnx_pcie_priv;
	void *osal_pcie_dev_ctx;
	struct nrf_wifi_ctx_lnx *lnx_rpu_ctx;

	struct pci_dev *pdev;

	struct nrf_wifi_osal_host_map host_map;
#ifdef SOC_WEZEN
#ifdef INLINE_RX
	struct nrf_wifi_osal_host_map host_map_inline_rx;
#endif
#endif
	void *intr_callbk_data;
	int (*intr_callbk_fn)(void *intr_callbk_data);

	char *dev_name;
	bool is_msi;

	bool dev_added;
	bool dev_init;
};


struct lnx_shim_llist_node{
	struct list_head head;
	void *data;
};

struct lnx_shim_llist{
	struct list_head head;
	unsigned int len;
};

#endif /* __LNX_SHIM_H__ */
