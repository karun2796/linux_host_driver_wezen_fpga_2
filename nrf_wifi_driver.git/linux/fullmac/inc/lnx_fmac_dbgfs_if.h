/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LNX_DBGFS_IF_H__
#define __LNX_DBGFS_IF_H__

#include "lnx_main.h"

#ifdef WLAN_SUPPORT
#include "lnx_fmac_main.h"
#endif /* WLAN_SUPPORT */

int nrf_wifi_lnx_dbgfs_init(void);
void nrf_wifi_lnx_dbgfs_deinit(void);
#ifdef WLAN_SUPPORT
int nrf_wifi_lnx_wlan_fmac_dbgfs_init(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_lnx_wlan_fmac_dbgfs_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
int nrf_wifi_lnx_wlan_fmac_dbgfs_stats_init(struct dentry *root,
				       struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_lnx_wlan_fmac_dbgfs_stats_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
int nrf_wifi_lnx_wlan_fmac_dbgfs_ver_init(struct dentry *root,
			             struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_lnx_wlan_fmac_dbgfs_ver_deinit(void);
#ifdef CONF_SUPPORT
void nrf_wifi_lnx_wlan_fmac_conf_init(struct rpu_conf_params *conf_params);
int nrf_wifi_lnx_wlan_fmac_dbgfs_conf_init(struct dentry *root,
				      struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_lnx_wlan_fmac_dbgfs_conf_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
#endif /* CONF_SUPPORT */
#ifdef CMD_DEMO
int nrf_wifi_lnx_wlan_fmac_dbgfs_connect_init(struct dentry *root,
        struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_lnx_wlan_fmac_dbgfs_connect_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
#endif
#endif /* WLAN_SUPPORT */
#ifdef TWT_SUPPORT
int nrf_wifi_lnx_wlan_fmac_dbgfs_twt_init(struct dentry *root,
				     struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_lnx_wlan_fmac_dbgfs_twt_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
#endif

#endif /* __LNX_DBGFS_IF_H__ */
