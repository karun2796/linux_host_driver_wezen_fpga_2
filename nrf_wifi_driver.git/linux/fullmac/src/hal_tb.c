#ifdef HAL_TB
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <pcie.h>
#include "lnx_hal_tb_dbgfs_if.h"
#include "pal.h"

#define NRF_WIFI_HAL_TB_PCIE_DRV_NAME "nrf_wifi_hal_tb_pcie"
#define NRF_WIFI_HAL_TB_PCIE_DRV_VER "1.0.0.0"

struct nrf_wifi_hal_tb_drv_priv_lnx rpu_hal_tb_drv_priv;


char *nrf_wifi_hal_tb_get_rpu_region_name_lnx(enum NRF_WIFI_REGION_TYPE region_type)
{
	switch (region_type) {
		case NRF_WIFI_REGION_TYPE_GRAM:
			return "GRAM";
		case NRF_WIFI_REGION_TYPE_PKTRAM:
			return "PKTRAM";
		case NRF_WIFI_REGION_TYPE_SYSBUS:
			return "SYSBUS";
		case NRF_WIFI_REGION_TYPE_PBUS:
			return "PBUS";
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_ROM:
			return "LMAC CORE ROM";
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_RET:
			return "LMAC CORE RETENTION";
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_SCRATCH:
			return "LMAC CORE SCRATCH";
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_ROM:
			return "UMAC CORE ROM";
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_RET:
			return "UMAC CORE RETENTION";
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_SCRATCH:
			return "UMAC CORE SCRATCH";
		default:
			pr_err("%s: Invalid RPU region type %d\n", __func__, region_type);
			return NULL;
	}
}


char *nrf_wifi_hal_tb_get_test_name_lnx(enum NRF_WIFI_HAL_TB_TEST_TYPE test_type)
{
	switch (test_type) {
		case NRF_WIFI_HAL_TB_TEST_TYPE_READ:
			return "READ";
		case NRF_WIFI_HAL_TB_TEST_TYPE_WRITE:
			return "WRITE";
		case NRF_WIFI_HAL_TB_TEST_TYPE_WRITE_READ:
			return "WRITE_READ";
		default:
			pr_err("%s: Invalid test type %d\n", __func__, test_type);
			return NULL;
	}
}


unsigned int nrf_wifi_hal_tb_get_region_addr_start(enum NRF_WIFI_REGION_TYPE region_type)
{
	switch (region_type) {
		case NRF_WIFI_REGION_TYPE_GRAM:
			return RPU_ADDR_GRAM_START;
		case NRF_WIFI_REGION_TYPE_PKTRAM:
			return RPU_ADDR_PKTRAM_START;
		case NRF_WIFI_REGION_TYPE_SYSBUS:
			return RPU_ADDR_SBUS_START;
		case NRF_WIFI_REGION_TYPE_PBUS:
			return RPU_ADDR_PBUS_START;
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_ROM:
			return RPU_ADDR_MCU1_CORE_ROM_START;
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_RET:
			return RPU_ADDR_MCU1_CORE_RET_START;
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_SCRATCH:
			return RPU_ADDR_MCU1_CORE_SCRATCH_START;
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_ROM:
			return RPU_ADDR_MCU2_CORE_ROM_START;
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_RET:
			return RPU_ADDR_MCU2_CORE_RET_START;
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_SCRATCH:
			return RPU_ADDR_MCU2_CORE_SCRATCH_START;
		default:
			pr_err("%s: Invalid RPU region type %d\n", __func__, region_type);
			return 0;
	}
}


unsigned int nrf_wifi_hal_tb_get_region_addr_end(enum NRF_WIFI_REGION_TYPE region_type)
{
	switch (region_type) {
		case NRF_WIFI_REGION_TYPE_GRAM:
			return RPU_ADDR_GRAM_END;
		case NRF_WIFI_REGION_TYPE_PKTRAM:
			return RPU_ADDR_PKTRAM_END;
		case NRF_WIFI_REGION_TYPE_SYSBUS:
			return RPU_ADDR_SBUS_END;
		case NRF_WIFI_REGION_TYPE_PBUS:
			return RPU_ADDR_PBUS_END;
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_ROM:
			return RPU_ADDR_MCU1_CORE_ROM_END;
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_RET:
			return RPU_ADDR_MCU1_CORE_RET_END;
		case NRF_WIFI_REGION_TYPE_LMAC_CORE_SCRATCH:
			return RPU_ADDR_MCU1_CORE_SCRATCH_END;
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_ROM:
			return RPU_ADDR_MCU2_CORE_ROM_END;
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_RET:
			return RPU_ADDR_MCU2_CORE_RET_END;
		case NRF_WIFI_REGION_TYPE_UMAC_CORE_SCRATCH:
			return RPU_ADDR_MCU2_CORE_SCRATCH_END;
		default:
			pr_err("%s: Invalid RPU region type %d\n", __func__, region_type);
			return 0;
	}
}


static void *nrf_wifi_hal_tb_dev_add_lnx(void *callbk_data,
					void *rpu_hal_ctx)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;
	
	rpu_hal_tb_ctx_lnx = kzalloc(sizeof(*rpu_hal_tb_ctx_lnx), GFP_KERNEL);

	if(!rpu_hal_tb_ctx_lnx) {
		pr_err("%s: Unable to allocate rpu_hal_tb_ctx_lnx\n", __func__);
		goto out;
	}

	rpu_hal_tb_ctx_lnx->rpu_hal_ctx = rpu_hal_ctx;

out:
	rpu_hal_tb_drv_priv.rpu_hal_tb_ctx_lnx[0] = rpu_hal_tb_ctx_lnx;
	return rpu_hal_tb_ctx_lnx;
}


static void nrf_wifi_hal_tb_dev_rem_lnx(void *rpu_ctx)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;

	rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)rpu_ctx;

	if (rpu_hal_tb_ctx_lnx->read_data) {
		kfree(rpu_hal_tb_ctx_lnx->read_data);
		rpu_hal_tb_ctx_lnx->read_data = NULL;
	}

	kfree(rpu_hal_tb_ctx_lnx);

	rpu_hal_tb_drv_priv.rpu_hal_tb_ctx_lnx[0] = NULL;
}


static enum nrf_wifi_status nrf_wifi_hal_tb_dev_init_lnx(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;

	rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)rpu_ctx;

	status = nrf_wifi_lnx_rpu_hal_tb_dbgfs_init(rpu_hal_tb_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s: Failed to create wlan entry in DebugFS\n",
		       __func__);
		goto out;
	}
out:
	return status;
}


static void nrf_wifi_hal_tb_dev_deinit_lnx(void *rpu_ctx)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;

	rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)rpu_ctx;

	nrf_wifi_lnx_rpu_hal_tb_dbgfs_deinit(rpu_hal_tb_ctx_lnx);
}


static int nrf_wifi_hal_tb_intr_callbk_lnx(void *rpu_ctx,
					  void *event_data,
					  unsigned int len)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;

	rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)rpu_ctx;

	rpu_hal_tb_ctx_lnx->num_intr_recd++;

	return NRF_WIFI_STATUS_SUCCESS;
}


int __init nrf_wifi_hal_tb_init_lnx(void)
{
	int ret = -ENOMEM;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	rpu_hal_tb_drv_priv.opriv = nrf_wifi_osal_init();

	if (!rpu_hal_tb_drv_priv.opriv) {
		pr_err("%s: nrf_wifi_osal_init failed\n", __func__);
		goto out;
	}

	ret = nrf_wifi_lnx_dbgfs_init();

	if (ret) {
		pr_err("%s: Failed to create root entry in DebugFS\n",
		       __func__);
		goto out;
	}

	rpu_hal_tb_drv_priv.hpriv = nrf_wifi_hal_init(rpu_hal_tb_drv_priv.opriv,
						     &rpu_hal_tb_drv_priv,
						     &nrf_wifi_hal_tb_dev_add_lnx,
						     &nrf_wifi_hal_tb_dev_rem_lnx,
						     &nrf_wifi_hal_tb_dev_init_lnx,
						     &nrf_wifi_hal_tb_dev_deinit_lnx,
						     &nrf_wifi_hal_tb_intr_callbk_lnx);

	if (rpu_hal_tb_drv_priv.hpriv == NULL) {
		pr_err("%s: nrf_wifi_hal_init failed\n", __func__);
		goto out;
	}

	status = nrf_wifi_hal_reg_drv(rpu_hal_tb_drv_priv.hpriv,
				     NRF_WIFI_HAL_TB_PCIE_DRV_NAME,
				     DEFAULT_IMGPCI_VENDOR_ID,
				     PCI_ANY_ID,
				     DEFAULT_IMGPCI_DEVICE_ID,
				     PCI_ANY_ID);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s: nrf_wifi_hal_reg_drv failed\n", __func__);
		nrf_wifi_hal_deinit(rpu_hal_tb_drv_priv.hpriv);
		nrf_wifi_osal_deinit(rpu_hal_tb_drv_priv.opriv);
		goto out;
	}
	
	ret = 0;
out:
	return ret;
}


void __exit nrf_wifi_hal_tb_deinit_lnx(void)
{
	nrf_wifi_hal_unreg_drv(rpu_hal_tb_drv_priv.hpriv);
	nrf_wifi_hal_deinit(rpu_hal_tb_drv_priv.hpriv);
	nrf_wifi_osal_deinit(rpu_hal_tb_drv_priv.opriv);
	nrf_wifi_lnx_dbgfs_deinit();
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nordic Semiconductor");
MODULE_DESCRIPTION("HAL TB Driver for Nordic RPU");
MODULE_VERSION(NRF_WIFI_HAL_TB_PCIE_DRV_VER);

module_init(nrf_wifi_hal_tb_init_lnx);
module_exit(nrf_wifi_hal_tb_deinit_lnx);
#endif /* HAL_TB */
