#ifdef HOST_FW_RAM_LOAD_SUPPORT
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing RAM loader specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */
#include "host_rpu_common_if.h"
#include "hal_fw_ram_loader.h"
#include "hal_mem.h"

/*
 * Reads the Header from the FW_RAM file and save it into context.
 * Updates pointer to the first Binary Record Structure in the FW_RAM file.
 */
static enum nrf_wifi_status nrf_wifi_hal_fw_ram_hdr_vldt(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
							   void *hdr)
{
	struct nrf_wifi_hal_fw_ram_hdr *fw_ram_hdr = NULL;

	fw_ram_hdr = (struct nrf_wifi_hal_fw_ram_hdr *)hdr;

	/* Some sanity checks on the FW_RAM file we've received as this needs
	 * to correspond to our parsing code.
	 */
	if ((fw_ram_hdr->ver_maj != 0) ||
	    (fw_ram_hdr->ver_min != 0x1000)) {
		/* Version error compared to our code */
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}


/*
 * Load data from the FW_RAM file into the specified memory region.
 */
static enum nrf_wifi_status nrf_wifi_hal_fw_ram_data_load(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
							    struct nrf_wifi_hal_fw_ram_bin_rec *fw_ram_bin_rec,
							    void **fw_ram_ptr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = hal_rpu_mem_write(hal_dev_ctx,
				   fw_ram_bin_rec->dest,
				   *fw_ram_ptr,
				   fw_ram_bin_rec->cmd_arg);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "%s: hal_rpu_mem_write failed\n",
				       __func__);
		goto out;
	}

	/* Once the associated load data has been loaded, the pointer is
	 * updated to the next Binary Record.
	 * Data is word aligned, so padding is done if necessary.
	 */
	if (fw_ram_bin_rec->cmd_arg % 4 == 0)
		*fw_ram_ptr += fw_ram_bin_rec->cmd_arg;
	else
		*fw_ram_ptr += fw_ram_bin_rec->cmd_arg +
			(4 - (fw_ram_bin_rec->cmd_arg % 4));

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


/*
 * Load MCP related data from the FW_RAM file into the specified memory region.
 */
static enum nrf_wifi_status nrf_wifi_hal_fw_ram_mcp_data_load(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
								struct nrf_wifi_hal_fw_ram_bin_rec *fw_ram_bin_rec,
								void **fw_ram_ptr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int size = 0;
	unsigned char mcp_id = 0;
	unsigned int num_words = 0;
	unsigned int addr_reg = 0;
	unsigned int data_reg = 0;
	unsigned int addr = 0;
	unsigned int data = 0;
	unsigned int i = 0;

	size = (fw_ram_bin_rec->cmd_arg & 0x00FFFFFF);
	mcp_id = ((fw_ram_bin_rec->cmd_arg & 0xFF000000) >> 24);

	if ((size % 12) != 0) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "%s: MCP Core data size is not a multiple of 3 words\n",
				       __func__);
		goto out;
	}

	num_words = (size / sizeof(int));

	if (mcp_id == 1) {
		addr_reg = RPU_REG_MCP_SYS_CSTRCTRL;
		data_reg = RPU_REG_MCP_SYS_CSTRDAT32;
	} else if (mcp_id == 2) {
		addr_reg = RPU_REG_MCP2_SYS_CSTRCTRL;
		data_reg = RPU_REG_MCP2_SYS_CSTRDAT32;
	} else if (mcp_id == 4) {
		addr_reg = RPU_REG_MCP3_SYS_CSTRCTRL;
		data_reg = RPU_REG_MCP3_SYS_CSTRDAT32;
	}

	addr = fw_ram_bin_rec->dest;

	status = hal_rpu_reg_write(hal_dev_ctx,
				   addr_reg,
				   addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "%s: hal_rpu_reg_write failed\n",
				       __func__);
		goto out;
	}

	for (i = 0; i < num_words; i++) {
		data = *((unsigned int *)(*fw_ram_ptr) + i);

		status = hal_rpu_reg_write(hal_dev_ctx,
					   data_reg,
					   data);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					       "%s: hal_rpu_reg_write failed\n",
					       __func__);
			goto out;
		}
	}

	/* Once the associated load data has been loaded, the pointer
	 * is updated to the next Binary Record.
	 * Data is word aligned, so padding is done if necessary.
	 */
	if (fw_ram_bin_rec->cmd_arg % 4 == 0)
		*fw_ram_ptr += size;
	else
		*fw_ram_ptr += size + (4 - fw_ram_bin_rec->cmd_arg % 4);

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

/*
 * Parses the firmware RAM image and loads it on the RPU.
 */
enum nrf_wifi_status nrf_wifi_hal_fw_ram_load(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						enum RPU_PROC_TYPE rpu_proc,
						void *fw_ram_data,
						unsigned int fw_ram_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	/* Set the HAL RPU context to the current required context */
	hal_dev_ctx->curr_proc = rpu_proc;
	
	void *fw_ram_ptr = NULL;
	struct nrf_wifi_hal_fw_ram_hdr *fw_ram_hdr = NULL;
	struct nrf_wifi_hal_fw_ram_bin_rec *fw_ram_bin_rec = NULL;
	unsigned int curr_addr = 0;
	unsigned int i = 0;
	unsigned int val = 0;

	fw_ram_ptr = fw_ram_data;
	fw_ram_hdr = fw_ram_ptr;
	status = nrf_wifi_hal_fw_ram_hdr_vldt(hal_dev_ctx,
					       fw_ram_hdr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				       "FW RAM header validation failed\n");
		goto out;
	}

	fw_ram_ptr += sizeof(*fw_ram_hdr);

	/* Read the rest of binary records and execute the given command. */
	while (1) {
		fw_ram_bin_rec = fw_ram_ptr;

		/* Execute commands */
		switch (fw_ram_bin_rec->cmd) {
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_DATA_LOAD:
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_DATA_LOAD_COLD:
			fw_ram_ptr += sizeof(*fw_ram_bin_rec);

			status = nrf_wifi_hal_fw_ram_data_load(hal_dev_ctx,
								fw_ram_bin_rec,
								&fw_ram_ptr);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
						       "%s: NRF_WIFI_HAL_FW_RAM_CMD_TYPE_DATA_LOAD failed\n",
						       __func__);

				goto out;
			}

			break;
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_REG_POKE:
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_REG_POKE_COLD:
			status = hal_rpu_reg_write(hal_dev_ctx,
						   fw_ram_bin_rec->dest,
						   fw_ram_bin_rec->cmd_arg);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
						       "%s: NRF_WIFI_HAL_FW_RAM_CMD_TYPE_REG_POKE failed\n",
						       __func__);

				goto out;
			}

			fw_ram_ptr += sizeof(*fw_ram_bin_rec);

			break;
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_MCP_CODE_LOAD:
			fw_ram_ptr += sizeof(*fw_ram_bin_rec);

			status = nrf_wifi_hal_fw_ram_mcp_data_load(hal_dev_ctx,
								    fw_ram_bin_rec,
								    &fw_ram_ptr);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
						       "%s: NRF_WIFI_HAL_FW_RAM_CMD_TYPE_MCP_CODE_LOAD failed\n",
						       __func__);

				goto out;
			}

			break;
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_ZERO_MEM:
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_ZERO_MEM_COLD:
			val = 0;

			for (i = 0; i < (fw_ram_bin_rec->cmd_arg / sizeof(int)); i++) {
				curr_addr = fw_ram_bin_rec->dest + (i * sizeof(int));

				status = hal_rpu_mem_write(hal_dev_ctx,
							   curr_addr,
							   &val,
							   sizeof(int));

				if (status != NRF_WIFI_STATUS_SUCCESS) {
					nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
							       "%s: NRF_WIFI_HAL_FW_RAM_CMD_TYPE_MCP_CODE_LOAD failed\n",
							       __func__);

					goto out;
				}
			}

			fw_ram_ptr += sizeof(*fw_ram_bin_rec);

			break;
		case NRF_WIFI_HAL_FW_RAM_CMD_TYPE_END_OF_LOAD:
			status = NRF_WIFI_STATUS_SUCCESS;
			goto out;

			break;	
		default:
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					       "%s: Invalid command in FW_RAM file (%d)\n",
					       fw_ram_bin_rec->cmd);
			status = NRF_WIFI_STATUS_FAIL;
			break;
		}
	}
out:
	/* Reset the HAL RPU context to the LMAC context */
	hal_dev_ctx->curr_proc = RPU_PROC_TYPE_MCU_LMAC;

	return status;	
}


enum nrf_wifi_status nrf_wifi_hal_fw_ram_boot(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
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
#endif /* HOST_FW_RAM_LOAD_SUPPORT */
