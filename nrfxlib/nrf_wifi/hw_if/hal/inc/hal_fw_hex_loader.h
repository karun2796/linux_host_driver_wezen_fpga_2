#ifdef HOST_FW_HEX_LOAD_SUPPORT
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing RAM loader specific declarations for the
 * HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_FW_HEX_LOADER_H__
#define __HAL_FW_HEX_LOADER_H__

#include "hal_structs.h"

/*
 * Parses the firmware RAM image and loads it on the RPU.
 */
enum nrf_wifi_status nrf_wifi_hal_fw_hex_load(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						void *fw_hex_data,
						unsigned int fw_hex_size);

long nrf_wifi_hal_get_fw_hex_patch_addr(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					enum RPU_PROC_TYPE rpu_proc,
					void *fw_hex_data,
					unsigned int fw_hex_size);
#endif /* __HAL_FW_HEX_LOADER_H__ */
#endif /* HOST_FW_HEX_LOAD_SUPPORT */
