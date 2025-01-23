#ifdef HAL_TB
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __HAL_TB_H__
#define __HAL_TB_H__

#include <linux/debugfs.h>
#include "hal_api.h"

#define MAX_NUM_RPU 1

enum NRF_WIFI_HAL_TB_WD_PATTERN_TYPE {
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_ALL_0,
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_ALL_F,
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_0A,
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_AB,
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_INC_NUM,
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_CUSTOM,
	NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_MAX
};


enum NRF_WIFI_HAL_TB_TEST_TYPE {
	NRF_WIFI_HAL_TB_TEST_TYPE_READ,
	NRF_WIFI_HAL_TB_TEST_TYPE_WRITE,
	NRF_WIFI_HAL_TB_TEST_TYPE_WRITE_READ,
	NRF_WIFI_HAL_TB_TEST_TYPE_MAX
};


struct nrf_wifi_hal_tb_ctx_lnx {
	void *rpu_hal_ctx;
	struct dentry *dbgfs_rpu_hal_tb_root;
	struct dentry *dbgfs_rpu_hal_tb_conf_root;
	struct dentry *dbgfs_rpu_hal_tb_stats_root;
	enum NRF_WIFI_REGION_TYPE rpu_region_type;
	enum NRF_WIFI_HAL_TB_WD_PATTERN_TYPE wd_pattern;
	unsigned int custom_data;
	unsigned int addr_offset;
	unsigned int data_size;
	unsigned char *read_data;
	enum NRF_WIFI_HAL_TB_TEST_TYPE test_type;
	unsigned int num_intr_recd;
};


struct nrf_wifi_hal_tb_drv_priv_lnx {
	struct dentry *dbgfs_root;
	struct nrf_wifi_osal_priv *opriv;
	struct nrf_wifi_hal_priv *hpriv;
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx[MAX_NUM_RPU];
};

char *nrf_wifi_hal_tb_get_rpu_region_name_lnx(enum NRF_WIFI_REGION_TYPE region_type);
char *nrf_wifi_hal_tb_get_test_name_lnx(enum NRF_WIFI_HAL_TB_TEST_TYPE test_type);
unsigned int nrf_wifi_hal_tb_get_region_addr_start(enum NRF_WIFI_REGION_TYPE region_type);
unsigned int nrf_wifi_hal_tb_get_region_addr_end(enum NRF_WIFI_REGION_TYPE region_type);
#endif /* __HAL_TB_H__ */
#endif /* HAL_TB */
