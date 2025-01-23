#ifdef HAL_TB
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/etherdevice.h>
#include "lnx_hal_tb_dbgfs_if.h"
#include "lnx_util.h"

#define MAX_CONF_BUF_SIZE 80
#define MAX_ERR_STR_SIZE 80

static __always_inline unsigned char param_get_val_uint(unsigned char *buf,
							unsigned char *str,
							unsigned int *val)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		if (!kstrtouint(temp, 0, val)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}


static __always_inline unsigned char param_get_val_int(unsigned char *buf,
						       unsigned char *str,
						       int *sval)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		if (!kstrtoint(temp, 0, sval)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}


static __always_inline unsigned char param_get_match(unsigned char *buf,
						     unsigned char *str)
{

	if (strstr(buf, str))
		return 1;
	else
		return 0;
}


static enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_validate_params(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	unsigned int start_addr = 0;
	unsigned int end_addr = 0;

	if ((rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_LMAC_CORE_ROM) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_LMAC_CORE_RET) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_LMAC_CORE_SCRATCH) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_UMAC_CORE_ROM) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_UMAC_CORE_RET) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_UMAC_CORE_SCRATCH)) {
		if (rpu_hal_tb_ctx_lnx->test_type != NRF_WIFI_HAL_TB_TEST_TYPE_WRITE) {
			pr_err("%s: Core memory is not readable\n", __func__);
			return NRF_WIFI_STATUS_FAIL;
		}
	}

	if (!rpu_hal_tb_ctx_lnx->data_size) {
		pr_err("%s: Data size not set\n",
		       __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	if ((rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_SYSBUS) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_PBUS)) {
		if (rpu_hal_tb_ctx_lnx->wd_pattern != NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_CUSTOM) {
			pr_err("%s: Only custom data pattern supported for region (%s)\n",
			       __func__,
			       nrf_wifi_hal_tb_get_rpu_region_name_lnx(rpu_hal_tb_ctx_lnx->rpu_region_type));
			return NRF_WIFI_STATUS_FAIL;
		}

		if (rpu_hal_tb_ctx_lnx->data_size != sizeof(int)) {
			pr_err("%s: Only 4 byte read/writes supported for region (%s)\n",
			       __func__,
			       nrf_wifi_hal_tb_get_rpu_region_name_lnx(rpu_hal_tb_ctx_lnx->rpu_region_type));
			return NRF_WIFI_STATUS_FAIL;
		}

		if ((rpu_hal_tb_ctx_lnx->addr_offset % sizeof(int)) != 0) {
			pr_err("%s: Only 4 byte aligned read/write addresses supported for region (%s)\n",
			       __func__,
			       nrf_wifi_hal_tb_get_rpu_region_name_lnx(rpu_hal_tb_ctx_lnx->rpu_region_type));
			return NRF_WIFI_STATUS_FAIL;
		}
	}

	start_addr = nrf_wifi_hal_tb_get_region_addr_start(rpu_hal_tb_ctx_lnx->rpu_region_type);

	if (!start_addr) {
		pr_err("%s: Invalid region %d\n", __func__, rpu_hal_tb_ctx_lnx->rpu_region_type);
		return NRF_WIFI_STATUS_FAIL;
	}

	end_addr = nrf_wifi_hal_tb_get_region_addr_end(rpu_hal_tb_ctx_lnx->rpu_region_type);

	if (!end_addr) {
		pr_err("%s: Invalid region %d\n", __func__, rpu_hal_tb_ctx_lnx->rpu_region_type);
		return NRF_WIFI_STATUS_FAIL;
	}

	if ((start_addr + rpu_hal_tb_ctx_lnx->addr_offset + rpu_hal_tb_ctx_lnx->data_size) > end_addr) {
		pr_err("%s: start_addr 0x%X,  end_addr 0x%X\n",
		       __func__,
		       start_addr,
		       end_addr);

		pr_err("%s: Invalid offset %d or size %d\n",
		       __func__,
		       rpu_hal_tb_ctx_lnx->addr_offset,
		       rpu_hal_tb_ctx_lnx->data_size);

		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}


static enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_test_read(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int start_addr = 0;
	unsigned int addr = 0;

	start_addr = nrf_wifi_hal_tb_get_region_addr_start(rpu_hal_tb_ctx_lnx->rpu_region_type);

	if (!start_addr) {
		pr_err("%s: Invalid region %d\n", __func__, rpu_hal_tb_ctx_lnx->rpu_region_type);
		return NRF_WIFI_STATUS_FAIL;
	}

	addr = start_addr + rpu_hal_tb_ctx_lnx->addr_offset;

	rpu_hal_tb_ctx_lnx->read_data = kmalloc(rpu_hal_tb_ctx_lnx->data_size, GFP_KERNEL);

	if (!rpu_hal_tb_ctx_lnx->read_data) {
		pr_err("%s: Insufficient memory available to read data\n", __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	if ((rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_SYSBUS) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_PBUS)) {
		status = hal_rpu_reg_read(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					  (unsigned int *)rpu_hal_tb_ctx_lnx->read_data,
					  addr);
	} else {
		status = hal_rpu_mem_read(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					  rpu_hal_tb_ctx_lnx->read_data,
					  addr,
					  rpu_hal_tb_ctx_lnx->data_size);
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s: hal_rpu_mem_read failed\n", __func__);
		goto out;
	}

out:
	return status;	
}


static enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_wd_construct(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx,
							       unsigned char *write_data)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int *write_data_int = NULL;
	unsigned int i = 0;

	switch (rpu_hal_tb_ctx_lnx->wd_pattern) {
		case NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_ALL_0:
			memset(write_data, 0x00, rpu_hal_tb_ctx_lnx->data_size);
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		case NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_ALL_F:
			memset(write_data, 0xFF, rpu_hal_tb_ctx_lnx->data_size);
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		case NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_0A:
			memset(write_data, 0x0A, rpu_hal_tb_ctx_lnx->data_size);
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		case NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_AB:
			memset(write_data, 0xAB, rpu_hal_tb_ctx_lnx->data_size);
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		case NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_INC_NUM:
			for (i = 0; i < rpu_hal_tb_ctx_lnx->data_size; i++)
				*(write_data + i) = i;
			status = NRF_WIFI_STATUS_SUCCESS;
			break;
		case NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_CUSTOM:
			write_data_int = (unsigned int *)write_data;

			for (i = 0; i < (rpu_hal_tb_ctx_lnx->data_size / sizeof(int)); i++)
				*(write_data_int + i) = rpu_hal_tb_ctx_lnx->custom_data;

			memcpy((write_data_int + i),
			       &rpu_hal_tb_ctx_lnx->custom_data,
			       (rpu_hal_tb_ctx_lnx->data_size % sizeof(int)));

			status = NRF_WIFI_STATUS_SUCCESS;

			break;
		default:
			pr_err("%s: Invalid data pattern type (%d)\n", __func__, rpu_hal_tb_ctx_lnx->wd_pattern);
			break;
	}

	return status;
}


static enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_test_write(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *write_data = NULL;
	unsigned int start_addr = 0;
	unsigned int addr = 0;

	start_addr = nrf_wifi_hal_tb_get_region_addr_start(rpu_hal_tb_ctx_lnx->rpu_region_type);

	if (!start_addr) {
		pr_err("%s: Invalid region %d\n", __func__, rpu_hal_tb_ctx_lnx->rpu_region_type);
		return NRF_WIFI_STATUS_FAIL;
	}

	addr = start_addr + rpu_hal_tb_ctx_lnx->addr_offset;

	if ((rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_LMAC_CORE_ROM) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_LMAC_CORE_RET) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_LMAC_CORE_SCRATCH)) 
		nrf_wifi_hal_proc_ctx_set(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					   RPU_PROC_MCU_LMAC);
	else if ((rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_UMAC_CORE_ROM) ||
		 (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_UMAC_CORE_RET) ||
		 (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_UMAC_CORE_SCRATCH)) 
		nrf_wifi_hal_proc_ctx_set(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					   RPU_PROC_MCU_UMAC);
	else
		nrf_wifi_hal_proc_ctx_set(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					   RPU_PROC_MAX);

	write_data = kmalloc(rpu_hal_tb_ctx_lnx->data_size, GFP_KERNEL);

	if (!write_data) {
		pr_err("%s: Insufficient memory available for write data\n", __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	status = nrf_wifi_lnx_rpu_hal_tb_wd_construct(rpu_hal_tb_ctx_lnx,
						   write_data);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s: nrf_wifi_lnx_rpu_hal_tb_wd_construct failed\n", __func__);
		goto out;
	}

	if ((rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_SYSBUS) ||
	    (rpu_hal_tb_ctx_lnx->rpu_region_type == NRF_WIFI_REGION_TYPE_PBUS)) {
		status = hal_rpu_reg_write(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					   addr,
					   *(unsigned int *)write_data);
	} else {
		status = hal_rpu_mem_write(rpu_hal_tb_ctx_lnx->rpu_hal_ctx,
					   addr,
					   write_data,
					   rpu_hal_tb_ctx_lnx->data_size);
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s: hal_rpu_mem_write failed\n", __func__);
		goto out;
	}
out:
	if (write_data) {
		kfree(write_data);
		write_data = NULL;
	}

	return status;
}


static enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_test_run(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = nrf_wifi_lnx_rpu_hal_tb_validate_params(rpu_hal_tb_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s: Invalid test params\n", __func__);
		goto out;
	}

	if (rpu_hal_tb_ctx_lnx->read_data) {
		kfree(rpu_hal_tb_ctx_lnx->read_data);
		rpu_hal_tb_ctx_lnx->read_data = NULL;
	}

	switch (rpu_hal_tb_ctx_lnx->test_type) {
		case NRF_WIFI_HAL_TB_TEST_TYPE_READ:
			status = nrf_wifi_lnx_rpu_hal_tb_test_read(rpu_hal_tb_ctx_lnx);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				pr_err("%s: nrf_wifi_lnx_rpu_hal_tb_test_read failed \n", __func__);
				goto out;
			}

			break;
		case NRF_WIFI_HAL_TB_TEST_TYPE_WRITE:
			status = nrf_wifi_lnx_rpu_hal_tb_test_write(rpu_hal_tb_ctx_lnx);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				pr_err("%s: nrf_wifi_lnx_rpu_hal_tb_test_write failed \n", __func__);
				goto out;
			}

			break;
		case NRF_WIFI_HAL_TB_TEST_TYPE_WRITE_READ:
			status = nrf_wifi_lnx_rpu_hal_tb_test_write(rpu_hal_tb_ctx_lnx);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				pr_err("%s: nrf_wifi_lnx_rpu_hal_tb_test_write failed \n", __func__);
				goto out;
			}

			status = nrf_wifi_lnx_rpu_hal_tb_test_read(rpu_hal_tb_ctx_lnx);

			if (status != NRF_WIFI_STATUS_SUCCESS) {
				pr_err("%s: nrf_wifi_lnx_rpu_hal_tb_test_read failed \n", __func__);
				goto out;
			}

			break;
		default:
			pr_err("%s: Invalid test type (%d)\n", __func__, rpu_hal_tb_ctx_lnx->test_type);
			break;

	}
out:
	return status;
}


static int nrf_wifi_lnx_rpu_hal_tb_conf_disp(struct seq_file *m, void *v)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)m->private;

	seq_puts(m, "************* Configured Parameters ***********\n");

	seq_printf(m,
		   "RPU region : %s\n",
		   nrf_wifi_hal_tb_get_rpu_region_name_lnx(rpu_hal_tb_ctx_lnx->rpu_region_type));

	seq_printf(m,
		   "Address offset : 0x%X\n",
		   rpu_hal_tb_ctx_lnx->addr_offset);

	seq_printf(m,
		   "Write data pattern : %d\n",
		   rpu_hal_tb_ctx_lnx->wd_pattern);

	seq_printf(m,
		   "Custom data : 0x%X\n",
		   rpu_hal_tb_ctx_lnx->custom_data);

	seq_printf(m,
		   "Data size : %d\n",
		   rpu_hal_tb_ctx_lnx->data_size);

	seq_printf(m,
		   "Test type : %s\n",
		   nrf_wifi_hal_tb_get_test_name_lnx(rpu_hal_tb_ctx_lnx->test_type));

	return 0;
}


static ssize_t nrf_wifi_lnx_rpu_hal_tb_conf_write(struct file *file,
					       const char __user *in_buf,
					       size_t count,
					       loff_t *ppos)
{
	char *conf_buf = NULL;
	unsigned int val = 0;
	char err_str[MAX_ERR_STR_SIZE];
	ssize_t ret_val = count;
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)file->f_inode->i_private;

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		ret_val = -EINVAL;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		ret_val = -ENOMEM;
		goto error;
	}

	if (copy_from_user(conf_buf,
			   in_buf,
			   count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		ret_val = -EACCES;
		goto error;
	}

	conf_buf[count] = '\0';

	if (param_get_val_uint(conf_buf, "rpu_region_type=", &val)) {
		if (val >= NRF_WIFI_REGION_TYPE_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %d\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		rpu_hal_tb_ctx_lnx->rpu_region_type = val;
	} else if (param_get_val_uint(conf_buf, "wd_pattern=", &val)) {
		if (val >= NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %d\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		rpu_hal_tb_ctx_lnx->wd_pattern = val;
	} else if (param_get_val_uint(conf_buf, "custom_data=", &val)) {
		if (rpu_hal_tb_ctx_lnx->wd_pattern != NRF_WIFI_HAL_TB_WD_PATTERN_TYPE_CUSTOM) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid write data pattern type %d\n", rpu_hal_tb_ctx_lnx->wd_pattern);
			ret_val = -EINVAL;
			goto error;
		}

		rpu_hal_tb_ctx_lnx->custom_data = val;
	} else if (param_get_val_uint(conf_buf, "addr_offset=", &val)) {
		rpu_hal_tb_ctx_lnx->addr_offset = val;
	} else if (param_get_val_uint(conf_buf, "data_size=", &val)) {
		rpu_hal_tb_ctx_lnx->data_size = val;
	} else if (strstr(conf_buf, "trigger")) {
		status = nrf_wifi_lnx_rpu_hal_tb_test_run(rpu_hal_tb_ctx_lnx);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Test failed\n");
			ret_val = -EINVAL;
			goto error;
		}
	} else if (param_get_val_uint(conf_buf, "test_type=", &val)) {
		if (val >= NRF_WIFI_HAL_TB_TEST_TYPE_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid test type %d\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		rpu_hal_tb_ctx_lnx->test_type = val;
	} else {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Invalid parameter name: %s\n",
			 conf_buf);
		ret_val = -EINVAL;
		goto error;
	}

	goto out;
error:
	pr_err("Error condition: %s\n", err_str);
out:
	kfree(conf_buf);

	return ret_val;
}


static int nrf_wifi_lnx_rpu_hal_tb_conf_open(struct inode *inode, struct file *file)
{
	struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx = (struct nrf_wifi_hal_tb_ctx_lnx *)inode->i_private;

	return single_open(file,
			   nrf_wifi_lnx_rpu_hal_tb_conf_disp,
			   rpu_hal_tb_ctx_lnx);
}


static const struct file_operations fops_rpu_hal_tb_conf = {
	.open = nrf_wifi_lnx_rpu_hal_tb_conf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = nrf_wifi_lnx_rpu_hal_tb_conf_write,
	.release = single_release
};

enum nrf_wifi_status nrf_wifi_lnx_rpu_hal_tb_dbgfs_conf_init(struct dentry *root,
							   struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if ((!root) || (!rpu_hal_tb_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		goto fail;
	}

	rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_conf_root = debugfs_create_file("conf",
									     0644,
									     root,
									     rpu_hal_tb_ctx_lnx,
									     &fops_rpu_hal_tb_conf);

	if (!rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_conf_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		goto fail;
	}

	status = NRF_WIFI_STATUS_SUCCESS;

	goto out;

fail:
	nrf_wifi_lnx_rpu_hal_tb_dbgfs_conf_deinit(rpu_hal_tb_ctx_lnx);
out:
	return status;
}


void nrf_wifi_lnx_rpu_hal_tb_dbgfs_conf_deinit(struct nrf_wifi_hal_tb_ctx_lnx *rpu_hal_tb_ctx_lnx)
{
	if (rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_conf_root)
		debugfs_remove(rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_conf_root);

	rpu_hal_tb_ctx_lnx->dbgfs_rpu_hal_tb_conf_root = NULL;
}
#endif /* HAL_TB */
