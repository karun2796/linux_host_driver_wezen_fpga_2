#ifdef CMD_DEMO
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/etherdevice.h>
#include "fmac_api.h"
#include "lnx_fmac_dbgfs_if.h"
#include "lnx_net_stack.h"
#include "lnx_util.h"


#define MAX_CONF_BUF_SIZE 80
#define MAX_ERR_STR_SIZE 80

void nrf_wifi_lnx_wlan_fmac_connect_init(rpu_connect_params_t *connect_params)
{
	memset(connect_params, 0, sizeof(*connect_params));
	/* Initialize values which are other than 0 */
}

static __always_inline unsigned char param_get_val(unsigned char *buf,
						   unsigned char *str,
						   unsigned long *val)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;
		if (!kstrtoul(temp, 0, val)) {
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

static ssize_t nrf_wifi_lnx_wlan_connect_write(struct file *file,
					    const char __user *in_buf,
					    size_t count,
					    loff_t *ppos)
{
	char *conf_buf = NULL;
	char err_str[MAX_ERR_STR_SIZE];
	size_t ret_val = count;
	unsigned long val = 0;

	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_ctx *fmac_ctx = NULL;
	struct nrf_wifi_umac_cmd_scan *scan_cmd = NULL;
	struct nrf_wifi_umac_cmd_disconn *deauth_cmd = NULL;

	rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)file->f_inode->i_private;
	fmac_ctx = (struct nrf_wifi_fmac_ctx *)(rpu_ctx_lnx->rpu_ctx);

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		ret_val = -EFAULT;
		goto error;
	}

	if (copy_from_user(conf_buf,
			   in_buf,
			   count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf[count-1] = '\0';

	if (param_get_val(conf_buf, "display_scan=", &val)) {
		if (val == 1) {
			if (rpu_ctx_lnx->connect_params.status != SCANNING) {
				struct nrf_wifi_umac_cmd_scan *scan_cmd = NULL;

				scan_cmd =  kzalloc(sizeof(*scan_cmd), GFP_KERNEL);

				if (!scan_cmd) {
					pr_err("%s: Unable to allocate memory\n", __func__);
					goto out;
				}
				scan_cmd->info.scan_mode = 0;
				scan_cmd->info.scan_reason = SCAN_DISPLAY;

				scan_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_TRIGGER_SCAN;
				scan_cmd->umac_hdr.ids.wdev_id = 0;
				scan_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

				status = umac_cmd_cfg(fmac_ctx,
						      scan_cmd,
						      sizeof(*scan_cmd));

				if (status == NRF_WIFI_STATUS_SUCCESS) {
					rpu_ctx_lnx->connect_params.status = SCANNING;
					rpu_ctx_lnx->connect_params.scan_type = SCAN_DISPLAY;
				}

				if (scan_cmd)
					kfree(scan_cmd);
				goto out;
			} else {
				pr_err("Already in Scan state\n");
			}
		} else {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}
	} else if (param_get_val(conf_buf, "get_display_scan_result=", &val)) {
		struct nrf_wifi_umac_cmd_get_scan_results *get_scan_results = NULL;

		if (rpu_ctx_lnx->display_scan_params) {
			int i;
			for (i = 0; i < rpu_ctx_lnx->display_scan_params->num; i++) {
				if (rpu_ctx_lnx->display_scan_params->res[i])
					kfree(rpu_ctx_lnx->display_scan_params->res[i]);
			}
			kfree(rpu_ctx_lnx->display_scan_params);
		}
		rpu_ctx_lnx->display_scan_params = 
			kzalloc(sizeof(struct rpu_display_scan_params), GFP_KERNEL);

		get_scan_results = kzalloc(sizeof(*get_scan_results), GFP_KERNEL);

		get_scan_results->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_GET_SCAN_RESULTS;
		get_scan_results->umac_hdr.ids.wdev_id = 0; //drv->ifindex; Hard Coded
		get_scan_results->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;
		get_scan_results->scan_reason = SCAN_DISPLAY;

		status = umac_cmd_cfg(fmac_ctx,
				      get_scan_results,
				      sizeof(*get_scan_results));
		if (get_scan_results) 
			kfree(get_scan_results);

		if (status == NRF_WIFI_STATUS_SUCCESS)
			goto out;
	} else if (param_get_val(conf_buf, "freq=", &val)) {
		rpu_ctx_lnx->connect_params.freq = val;
		goto out;
	} else if (param_get_match(conf_buf, "bssid=")) {
		//int len = (count - 5);
		//int i = 0;
		memset(rpu_ctx_lnx->connect_params.bssid, 0,
		       sizeof(rpu_ctx_lnx->connect_params.bssid));
		if (hex_str_to_val(rpu_ctx_lnx->connect_params.bssid,
				   sizeof(rpu_ctx_lnx->connect_params.bssid),
				   strstr(conf_buf, "=") + 1) == -1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value: Enter in format:001122334455\n");
			goto error;
		}
		goto out;
	} else if (param_get_match(conf_buf, "ssid=")) {
		int ssid_len = (count - 5);
		memcpy(&rpu_ctx_lnx->connect_params.ssid,
		       strstr(conf_buf, "=") + 1,
		       (ssid_len - 1));
		rpu_ctx_lnx->connect_params.ssid_len = (ssid_len - 1);
		goto out;
	} else if (param_get_val(conf_buf, "connect=", &val)) {
		if ((val < 0) ||  (val > 1)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		} else if (val == 1) {
			if ((rpu_ctx_lnx->connect_params.status == DISCONNECTED)
			    || (rpu_ctx_lnx->connect_params.status == SCAN_RESULTS_COMPLETE)) {

				struct nrf_wifi_channel *chnl;

				scan_cmd =  kzalloc(sizeof(*scan_cmd) + sizeof(struct nrf_wifi_channel),
						    GFP_KERNEL);

				if(!scan_cmd) {
					pr_err("%s: Unable to allocate memory\n", __func__);
					goto out;
				}

				scan_cmd->info.scan_params.num_scan_ssids = 1;
				if (rpu_ctx_lnx->connect_params.ssid_len) {
					memcpy(&scan_cmd->info.scan_params.scan_ssids[0].nrf_wifi_ssid,
					       rpu_ctx_lnx->connect_params.ssid,
					       rpu_ctx_lnx->connect_params.ssid_len);
					scan_cmd->info.scan_params.scan_ssids[0].nrf_wifi_ssid_len =
						rpu_ctx_lnx->connect_params.ssid_len;
				}
				scan_cmd->info.scan_params.num_scan_channels = 1;

				chnl =  &scan_cmd->info.scan_params.channels[0];
				chnl->center_frequency = rpu_ctx_lnx->connect_params.freq;

				memcpy(&scan_cmd->info.scan_params.mac_addr,
				       rpu_ctx_lnx->connect_params.bssid, ETH_ALEN);
				scan_cmd->info.scan_mode = 0;
				scan_cmd->info.scan_reason = SCAN_CONNECT;
				scan_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_TRIGGER_SCAN;
				scan_cmd->umac_hdr.ids.wdev_id = 0;
				scan_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

				status = umac_cmd_cfg(fmac_ctx,
						      scan_cmd,
						      sizeof(*scan_cmd) + sizeof(struct nrf_wifi_channel));

				if (status == NRF_WIFI_STATUS_SUCCESS) {
					rpu_ctx_lnx->connect_params.status = SCANNING;
					rpu_ctx_lnx->connect_params.scan_type = CONNECT_SCAN;
				}
				if (scan_cmd)
					kfree(scan_cmd);
			} else {
				pr_debug("Already in connected state\n");
			}
			goto out;
		} else if (val == 0) {
			if (rpu_ctx_lnx->connect_params.status == CONNECTED) {

				deauth_cmd = kzalloc(sizeof(*deauth_cmd), GFP_KERNEL);

				if(!deauth_cmd) {
					pr_err("%s: Unable to allocate memory\n", __func__);
					goto out;
				}

				deauth_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_DEAUTHENTICATE;
				deauth_cmd->umac_hdr.ids.wdev_id = 0;
				deauth_cmd->umac_hdr.ids.valid_fields |= NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

				memcpy(&deauth_cmd->info.mac_addr,
				       rpu_ctx_lnx->connect_params.bssid,
				       ETH_ALEN);
				deauth_cmd->valid_fields |= NRF_WIFI_CMD_MLME_MAC_ADDR_VALID;

				deauth_cmd->info.reason_code = WLAN_REASON_DEAUTH_LEAVING;

				status = umac_cmd_cfg(fmac_ctx,
						      deauth_cmd,
						      sizeof(*deauth_cmd));

				if (status == NRF_WIFI_STATUS_SUCCESS) {
					rpu_ctx_lnx->connect_params.status = DISCONNECTED;
					//    cfg80211_disconnected(netdev, WLAN_REASON_DEAUTH_LEAVING, NULL,0, true, GFP_KERNEL);
				}
			} else {
				pr_debug("Already in dis-connected state\n");
			}
			goto out;
		}
	}
error:
	pr_err("Error condition: %s\n", err_str);

out:    
	if (conf_buf) 
		kfree(conf_buf);

	return ret_val;
}


static int nrf_wifi_lnx_wlan_fmac_dbgfs_connect_show(struct seq_file *m, void *v)
{
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = NULL;
	rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)m->private;

	if (rpu_ctx_lnx->connect_params.ssid_len)
		seq_printf(m, "ssid= %s\n",  rpu_ctx_lnx->connect_params.ssid);
	else 
		seq_printf(m, "ssid= %s\n", "<null>");

	switch(rpu_ctx_lnx->connect_params.status) {
		case 0:
			seq_printf(m, "status = DISCONNECTED\n");
			break;
		case 1:
			seq_printf(m, "status = SCANNING\n");
			break;
		case 2:
			seq_printf(m, "status = SCAN_COMPLETE\n");
			break;
		case 3:
			seq_printf(m, "status = SCAN_RESULTS_COMPLETE\n");
			break;
		case 4:
			seq_printf(m, "status = CONNECTING\n");
			break;
		case 5:
			seq_printf(m, "status = CONNECTED\n");
			break;
	}
	return 0;
}


static int nrf_wifi_lnx_wlan_connect_open(struct inode *inode, struct file *file)
{

	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)inode->i_private;

	return single_open(file,
			   nrf_wifi_lnx_wlan_fmac_dbgfs_connect_show,
			   rpu_ctx_lnx);
}


static const struct file_operations fops_wlan_connect = {
	.open = nrf_wifi_lnx_wlan_connect_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = nrf_wifi_lnx_wlan_connect_write,
	.release = single_release
};


int nrf_wifi_lnx_wlan_fmac_dbgfs_connect_init(struct dentry *root,
					   struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	int ret = 0;
	nrf_wifi_lnx_wlan_fmac_connect_init(&rpu_ctx_lnx->connect_params);

	if ((!root) || (!rpu_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_ctx_lnx->dbgfs_nrf_wifi_connect_root = debugfs_create_file("connect",
									 0644,
									 root,
									 rpu_ctx_lnx,
									 &fops_wlan_connect);

	if (!rpu_ctx_lnx->dbgfs_nrf_wifi_connect_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_lnx_wlan_fmac_dbgfs_connect_deinit(rpu_ctx_lnx);
out:
	return ret;
}


void nrf_wifi_lnx_wlan_fmac_dbgfs_connect_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	printk("+%s \n",__func__);
	if (rpu_ctx_lnx->display_scan_params) {
		int i;
		for (i = 0; i < rpu_ctx_lnx->display_scan_params->num; i++) {
			if (rpu_ctx_lnx->display_scan_params->res[i])
				kfree(rpu_ctx_lnx->display_scan_params->res[i]);
		}
		kfree(rpu_ctx_lnx->display_scan_params);
	}
	if (rpu_ctx_lnx->dbgfs_nrf_wifi_connect_root)
		debugfs_remove(rpu_ctx_lnx->dbgfs_nrf_wifi_connect_root);

	rpu_ctx_lnx->dbgfs_nrf_wifi_connect_root = NULL;
	printk("-%s \n",__func__);
}

#endif /* CMD_DEMO */
