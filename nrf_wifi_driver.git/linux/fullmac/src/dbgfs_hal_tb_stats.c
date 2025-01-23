#ifdef HAL_TB
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lnx_hal_tb_dbgfs_if.h"
#include "hal_api.h"

static enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_show(struct seq_file *m, void *v)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int i = 0;
	unsigned int start_addr = 0;

	rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)m->private;

	start_addr = nrf_wifi_hal_tb_get_region_addr_start(rpu_hal_tb_ctx_lnx->rpu_region_type);

	if (!start_addr) {
		seq_printf(m, "Invalid region %d\n", rpu_hal_tb_ctx_lnx->rpu_region_type);
		goto out;
	}

	seq_printf(m, "\n************ Interrupts recd (%d) ************\n", rpu_hal_tb_ctx_lnx->num_intr_recd);

	seq_printf(m, "\n*************** Read Data (%s) ***************\n", nrf_wifi_hal_tb_get_rpu_region_name_lnx(rpu_hal_tb_ctx_lnx->rpu_region_type));

	if (rpu_hal_tb_ctx_lnx->read_data) {
		for (i = 0; i < rpu_hal_tb_ctx_lnx->data_size; i++) {
			if ((i % 16) == 0) {
				seq_printf(m,
					   "\n");
				seq_printf(m,
					   "%08X : ",
					   start_addr + rpu_hal_tb_ctx_lnx->addr_offset + i);
			}

			seq_printf(m,
				   "%02X ",
				   *(rpu_hal_tb_ctx_lnx->read_data + i));
		}
	} else {
		seq_printf(m, "No read data available\n");
	}

	seq_puts(m, "\n\n");

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


static int nrf_wifi_lnx_rpu_hal_tb_stats_open(struct inode *inode, struct file *file)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)inode->i_private;

	return single_open(file,
			   nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_show,
			   rpu_hal_tb_ctx_lnx);
}


static const struct file_operations fops_rpu_hal_tb_stats = {
	.open = nrf_wifi_lnx_rpu_hal_tb_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = NULL,
	.release = single_release
};


int nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_init(struct dentry *root,
					  struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	int ret = 0;

	if ((!root) || (!rpu_hal_tb_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_stats_root = debugfs_create_file("stats",
									      0444,
									      root,
									      rpu_hal_tb_ctx_lnx,
									      &fops_rpu_hal_tb_stats);

	if (!rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_stats_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_deinit(rpu_hal_tb_ctx_lnx);

out:
	return ret;
}


void nrf_wifi_lnx_rpu_hal_tb_dbgfs_stats_deinit(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	if (rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_stats_root)
		debugfs_remove(rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_stats_root);

	rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_stats_root = NULL;
}
#endif /* HAL_TB */
