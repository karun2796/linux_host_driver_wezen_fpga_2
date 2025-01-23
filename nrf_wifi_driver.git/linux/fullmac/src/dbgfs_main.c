/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef HAL_TB
#include "lnx_hal_tb_dbgfs_if.h"
extern struct nrf_wifi_hal_tb_drv_priv_lnx rpu_hal_tb_drv_priv;
#else
#include "lnx_fmac_dbgfs_if.h"
extern struct nrf_wifi_drv_priv_lnx rpu_drv_priv;
#endif /* !HAL_TB */


int nrf_wifi_lnx_dbgfs_init(void)
{
	int status = -1;

#ifdef HAL_TB
	rpu_hal_tb_drv_priv.dbgfs_root = debugfs_create_dir("nrf",
							    NULL);

	if (!rpu_hal_tb_drv_priv.dbgfs_root)	
		goto out;
#else
	rpu_drv_priv.dbgfs_root = debugfs_create_dir("nrf",
						     NULL);

	if (!rpu_drv_priv.dbgfs_root)	
		goto out;
#endif /* !HAL_TB */

	status = 0;
out:
	if (status)
		nrf_wifi_lnx_dbgfs_deinit();

	return status;
}


void nrf_wifi_lnx_dbgfs_deinit(void)
{
#ifdef HAL_TB
	if (rpu_hal_tb_drv_priv.dbgfs_root)
		debugfs_remove_recursive(rpu_hal_tb_drv_priv.dbgfs_root);

	rpu_hal_tb_drv_priv.dbgfs_root = NULL;
#else
	if (rpu_drv_priv.dbgfs_root)
		debugfs_remove_recursive(rpu_drv_priv.dbgfs_root);

	rpu_drv_priv.dbgfs_root = NULL;
#endif /* !HAL_TB */
}


#ifdef WLAN_SUPPORT
int nrf_wifi_lnx_wlan_fmac_dbgfs_init(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	rpu_ctx_lnx->dbgfs_wlan_root = debugfs_create_dir("wifi",
							  rpu_drv_priv.dbgfs_root);

	if (!rpu_ctx_lnx->dbgfs_wlan_root)
		goto out;

#ifdef CONF_SUPPORT
	status = nrf_wifi_lnx_wlan_fmac_dbgfs_conf_init(rpu_ctx_lnx->dbgfs_wlan_root,
						     rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;
#endif /* CONF_SUPPORT */

#ifdef TWT_SUPPORT
    status = nrf_wifi_lnx_wlan_fmac_dbgfs_twt_init(rpu_ctx_lnx->dbgfs_wlan_root,
						rpu_ctx_lnx);
 
    if (status != NRF_WIFI_STATUS_SUCCESS)
        goto out;
#endif /* TWT_SUPPORT */
	
#ifdef CMD_DEMO
	status = nrf_wifi_lnx_wlan_fmac_dbgfs_connect_init(rpu_ctx_lnx->dbgfs_wlan_root,
							rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;
#endif /* CMD_DEMO */



	status = nrf_wifi_lnx_wlan_fmac_dbgfs_stats_init(rpu_ctx_lnx->dbgfs_wlan_root,
						      rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

	status = nrf_wifi_lnx_wlan_fmac_dbgfs_ver_init(rpu_ctx_lnx->dbgfs_wlan_root,
						    rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

out:
	if (status != NRF_WIFI_STATUS_SUCCESS)
		nrf_wifi_lnx_wlan_fmac_dbgfs_deinit(rpu_ctx_lnx);

	return status;
}


void nrf_wifi_lnx_wlan_fmac_dbgfs_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_wlan_root)
		debugfs_remove_recursive(rpu_ctx_lnx->dbgfs_wlan_root);

	rpu_ctx_lnx->dbgfs_wlan_root = NULL;
}
#endif /* WLAN_SUPPORT */

#ifdef HAL_TB
int nrf_wifi_lnx_rpu_hal_tb_dbgfs_init(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root = debugfs_create_dir("hal_tb",
								       rpu_hal_tb_drv_priv.dbgfs_root);

	if (!rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root)
		goto out;

	status = nrf_wifi_lnx_rpu_hal_tb_dbgfs_conf_init(rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root,
						      rpu_hal_tb_ctx_lnx);			

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

	status = nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_init(rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root,
						       rpu_hal_tb_ctx_lnx);			

out:
	if (status != NRF_WIFI_STATUS_SUCCESS)
		nrf_wifi_lnx_rpu_hal_tb_dbgfs_deinit(rpu_hal_tb_ctx_lnx);

	return status;
}


void nrf_wifi_lnx_rpu_hal_tb_dbgfs_deinit(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	if (rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root)
		debugfs_remove_recursive(rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root);

	rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_root = NULL;
}
#endif /* HAL_TB */
