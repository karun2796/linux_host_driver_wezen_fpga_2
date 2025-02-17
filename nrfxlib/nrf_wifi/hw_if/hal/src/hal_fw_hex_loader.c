#ifdef HOST_FW_HEX_LOAD_SUPPORT
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing HEX loader specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */
#include <linux/kernel.h>

#include "host_rpu_common_if.h"
#include "hal_fw_hex_loader.h"
#include "hal_mem.h"

#define FWLDR_ADDR_LEN    8
#define FWLDR_BYTE_LEN    2

long parse_data(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
		unsigned char *fw_data_ptr)
{
	char buff[FWLDR_BYTE_LEN + 1];
	long val;

	nrf_wifi_osal_mem_cpy(hal_dev_ctx->hpriv->opriv,
			      buff,
			      fw_data_ptr,
			      FWLDR_BYTE_LEN);
	//memcpy(buff, fw_data_ptr, FWLDR_BYTE_LEN);
	buff[FWLDR_BYTE_LEN] = '\0';

	if (kstrtol(buff, 16, &val) != 0) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Unable to Convert\n",
				       __func__);
		return -1;
	}
	return val;
}

long parse_address(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
		   unsigned char *fw_data_ptr)
{
	unsigned int address = 0;
	char buff[FWLDR_ADDR_LEN + 1];
	long val;

	if (fw_data_ptr[0] != '@')
		return -1;

	nrf_wifi_osal_mem_cpy(hal_dev_ctx->hpriv->opriv,
			      buff,
			      fw_data_ptr + 1,
			      FWLDR_ADDR_LEN);
//	memcpy(buff, fw_data_ptr + 1, FWLDR_ADDR_LEN);
	buff[FWLDR_ADDR_LEN] = '\0';

	if (kstrtol(buff, 16, &val) != 0) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Unable to Convert\n",
				       __func__);
		return -1;
	}

	return val;
}

static enum nrf_wifi_status nrf_wifi_hal_fw_hex_load_data(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				    enum RPU_PROC_TYPE rpu_proc,
				    const unsigned char *fw_data,
				    unsigned int fw_data_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *fw_data_ptr = fw_data;
	unsigned int address = 0;
	unsigned int byte = 0;
	unsigned int word = 0;
	unsigned int byteCount = 0;
	unsigned int initPcAddress;
	size_t sz;
	long val;
	int len = 0;

	if (fw_data_ptr[0] != '@')
		goto out;

        val = parse_address(hal_dev_ctx, fw_data_ptr);

	if (val == -1)
		goto out;

	initPcAddress = (int)val;

	hal_dev_ctx->curr_proc = rpu_proc;
	for (sz = fw_data_size, fw_data_ptr = fw_data;
	     sz;
	     sz -= len, fw_data_ptr += len) {
		if (fw_data_ptr[0] == '\n' ||
		    fw_data_ptr[0] == '\r' ||
		    fw_data_ptr[0] == ' ') {
			/* set length to 1,
			 * to increment fw_data_ptr next by 1.
			 */
			len = 1;
		} else if (fw_data_ptr[0] == '@') {
			// Address
                        // Take care of left over unaligned data
                        if (byteCount != 0) {
				status = hal_rpu_mem_write(hal_dev_ctx,
							   address,
							   word,
							   4);
				if (status != NRF_WIFI_STATUS_SUCCESS) {
					nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
							      "%s: hal_rpu_mem_write failed\n",
							       __func__);
			                goto out;
			        }
			}

			val = parse_address(hal_dev_ctx, fw_data_ptr);
			if (val == -1) {
				status = NRF_WIFI_STATUS_FAIL;
				goto out;
			}

			address = (int)val;
			/* set length to 9, to increment fw_data_ptr next by 9.
			 * as first character is '@' and rest of the
			 * characters represent the address
			 */
			len = (1 + FWLDR_ADDR_LEN);
			word = 0;
			byteCount = 0;
			//nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv, "hal_rpu_mem_write-----address= %0x\n", address);
		} else {
			//Data
			val = parse_data(hal_dev_ctx, fw_data_ptr);

			if (val == -1) {
				status = NRF_WIFI_STATUS_FAIL;
				goto out;
			}

			byte = (int)val;

			/* set length to 2, to increment fw_data_ptr next by 2
			 * as 2 characters converted to 1 byte.
			 */

			len = FWLDR_BYTE_LEN;

			word |= byte << (byteCount *8);
			byteCount++;

			if (byteCount == 4) {
				status = hal_rpu_mem_write(hal_dev_ctx,
							   address,
							   word,
							   4);
				if (status != NRF_WIFI_STATUS_SUCCESS) {
					nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
							      "%s: hal_rpu_mem_write failed\n",
							       __func__);
			                goto out;
			        }
				word = 0;
				byteCount = 0;
				address += 4;
			}
		}
	}

	// Take care of left over unaligned data
	if (byteCount != 0) {
		status = hal_rpu_mem_write(hal_dev_ctx,
					   address,
					   word,
					   4);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: hal_rpu_mem_write failed\n",
					       __func__);
	                goto out;
	        }
	}

out:
	return status;
}

/*
 * Parses the firmware HEX image and loads it on the RPU.
 */
enum nrf_wifi_status nrf_wifi_hal_fw_hex_load(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						void *fw_data,
						unsigned int fw_data_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = nrf_wifi_hal_fw_hex_load_data(hal_dev_ctx, rpu_proc , fw_data, fw_data_size);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				"FW HEX Image loading failed\n");
		goto out;
	}
out:
	return status;

}

/*
 * Parse first  bytes for patch address
 */
long nrf_wifi_hal_get_fw_hex_patch_addr(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				    enum RPU_PROC_TYPE rpu_proc,
				    void *fw_data,
				    unsigned int fw_data_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *fw_data_ptr = fw_data;
	long val;

	if (fw_data_ptr[0] != '@')
		return -1;

	val = parse_address(hal_dev_ctx, fw_data_ptr);

	if (val == -1)
		return -1;

	return val;
}
#endif /* HOST_FW_HEX_LOAD_SUPPORT */
