#ifdef HAL_TB
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LNX_HAL_TB_DBGFS_IF_H__
#define __LNX_HAL_TB_DBGFS_IF_H__

#include "hal_tb.h"

int nrf_wifi_lnx_dbgfs_init(void);
void nrf_wifi_lnx_dbgfs_deinit(void);
int nrf_wifi_lnx_rpu_hal_tb_dbgfs_init(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx);
void nrf_wifi_lnx_rpu_hal_tb_dbgfs_deinit(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx);
int nrf_wifi_lnx_rpu_hal_tb_dbgfs_conf_init(struct dentry *root,
					 struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx);
void nrf_wifi_lnx_rpu_hal_tb_dbgfs_conf_deinit(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx);
int nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_init(struct dentry *root,
					  struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx);
void nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_deinit(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx);
#endif /* __LNX_HAL_TB_DBGFS_IF_H__ */
#endif /* HAL_TB */
