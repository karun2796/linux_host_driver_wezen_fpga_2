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
				status = hal_rpu_mem_code_write(hal_dev_ctx,
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
			pr_err("------Address = 0x%x----------\n", address);
			/* set length to 9, to increment fw_data_ptr next by 9.
			 * as first character is '@' and rest of the
			 * characters represent the address
			 */
			len = (1 + FWLDR_ADDR_LEN);
			word = 0;
			byteCount = 0;
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
				status = hal_rpu_mem_code_write(hal_dev_ctx,
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
		status = hal_rpu_mem_code_write(hal_dev_ctx,
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

	pr_err("Calling nrf_wifi_hal_fw_hex_load_data with rpu_poc= %d\n", rpu_proc);
	status = nrf_wifi_hal_fw_hex_load_data(hal_dev_ctx, rpu_proc , fw_data, fw_data_size);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				"FW HEX Image loading failed\n");
		goto out;
	}
out:
	return status;	

}

#if 0
enum nrf_wifi_status nrf_wifi_hal_fw_hex_boot(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						enum RPU_PROC_TYPE rpu_proc)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int boot_sig_addr = 0;
	unsigned int run_addr = 0;
	unsigned int val = 0;

	if (rpu_proc == RPU_PROC_TYPE_MCU_LMAC) {
		boot_sig_addr = RPU_MEM_LMAC_BOOT_SIG;
		run_addr = RPU_REG_MIPS_MCU_CONTROL;
	} else if (rpu_proc == RPU_PROC_TYPE_MCU_UMAC) {
		boot_sig_addr = RPU_MEM_UMAC_BOOT_SIG;
		run_addr = RPU_REG_MIPS_MCU2_CONTROL;
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "%s: Invalid RPU processor type %d\n",
				       __func__,
				       rpu_proc);
		goto out;
	}

	/* Set the HAL RPU context to the current required context */
	hal_dev_ctx->curr_proc = rpu_proc;

	/* Clear the firmware pass signature location */
	status = hal_rpu_mem_write(hal_dev_ctx,
				   boot_sig_addr,
				   &val,
				   sizeof(val));

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "%s: Clearing of FW pass signature failed for RPU(%d)\n",
				       __func__,
				       rpu_proc);

		goto out;
	}

	/* MIPS will be in its sleep state, on its wait instruction, the concept
	 * of running assumes the wait at the BEV has been replaced by a
	 * trampoline to real code, and to get the MIPS running again we need to
	 * reset it.
	 */

	/* Perform pulsed soft reset of MIPS - this should now run */
	status = hal_rpu_reg_write(hal_dev_ctx,
				   run_addr,
				   0x1);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "%s: RPU processor(%d) run failed\n",
				       __func__,
				       rpu_proc);

		goto out;
	}

out:
	/* Reset the HAL RPU context to the LMAC context */
	hal_dev_ctx->curr_proc = RPU_PROC_TYPE_MCU_LMAC;

	return status;	

}
#endif
#endif /* HOST_FW_HEX_LOAD_SUPPORT */
