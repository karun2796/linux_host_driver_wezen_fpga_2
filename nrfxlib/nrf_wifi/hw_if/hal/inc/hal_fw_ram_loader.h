#ifdef HOST_FW_RAM_LOAD_SUPPORT 
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing RAM loader specific declarations for the
 * HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_FW_RAM_LOADER_H__
#define __HAL_FW_RAM_LOADER_H__

#include "hal_structs.h"

enum NRF_WIFI_HAL_FW_RAM_CMD_TYPE {
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_DATA_LOAD = 0,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_REG_POKE,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_MCP_CODE_LOAD,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_END_OF_LOAD,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_ZERO_MEM,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_DATA_LOAD_COLD,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_REG_POKE_COLD,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_ZERO_MEM_COLD,
	NRF_WIFI_HAL_FW_RAM_CMD_TYPE_MAX
};


struct nrf_wifi_hal_fw_ram_hdr {
	unsigned int hdr_prefix;
	unsigned short ver_maj;
	unsigned short ver_min;
	unsigned int data_size;
	unsigned int exec_addr;
	unsigned int options_flags;
	unsigned int data_crc;
	unsigned int hdr_crc;
};


struct nrf_wifi_hal_fw_ram_bin_rec {
	unsigned int cmd_arg;
	unsigned int dest;
	unsigned short cmd;
	unsigned short hdr_crc;
};


/*
 * Parses the firmware RAM image and loads it on the RPU.
 */
enum nrf_wifi_status nrf_wifi_hal_fw_ram_load(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						void *fw_ram_data,
						unsigned int fw_ram_size);

enum nrf_wifi_status nrf_wifi_hal_fw_ram_boot(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						enum RPU_PROC_TYPE rpu_proc);
#endif /* __HAL_FW_RAM_LOADER_H__ */
#endif /* HOST_FW_RAM_LOAD_SUPPORT */
