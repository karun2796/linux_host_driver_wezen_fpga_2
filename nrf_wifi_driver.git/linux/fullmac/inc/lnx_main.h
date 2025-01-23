/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LNX_MAIN_H__
#define __LNX_MAIN_H__

#include <linux/debugfs.h>
#include "rpu_if.h"
#ifdef DEBUG_MODE_SUPPORT
#include "host_rpu_umac_if.h"
#endif

#ifdef WLAN_SUPPORT
#ifdef RPU_CONFIG_FMAC
#include <net/cfg80211.h>
#include "fmac_structs.h"
#endif /* RPU_CONFIG_FMAC */
#endif /* WLAN_SUPPORT */	

#define NRF_WIFI_FMAC_DRV_VER "1.0.0.0"

#ifndef HOST_CFG80211_SUPPORT
#define MAX_NUM_RPU 1
#endif /* !HOST_CFG80211_SUPPORT */
#ifdef TWT_SUPPORT
enum connect_status {
      DISCONNECTED,
     CONNECTED,
};

struct twt_params {
      unsigned char twt_flow_id;
      unsigned char neg_type;
      int setup_cmd;
	  unsigned char ap_trigger_frame;
      unsigned char is_implicit;
      unsigned char twt_flow_type;
      unsigned char twt_target_wake_interval_exponent;
      unsigned short  twt_target_wake_interval_mantissa;
      unsigned long long target_wake_time;
      unsigned short nominal_min_twt_wake_duration;
};

 struct rpu_twt_params {
      //unsigned char target_wake_time[8]; //should it be changed to unsigned long long
      enum connect_status status;
      struct twt_params twt_cmd;
      struct twt_params twt_event;
      int twt_event_info_avail;
      char twt_setup_cmd[250];
	  int teardown_reason;
	  int teardown_event_cnt;
};


#endif 

#ifdef CMD_DEMO
#define MAX_SSID_LEN 80
enum connect_status {
	DISCONNECTED,
	SCANNING,
	SCAN_COMPLETE,
	SCAN_RESULTS_COMPLETE,
	CONNECTING,
	CONNECTED,
};

enum scan_type {
	DISPLAY_SCAN,
	CONNECT_SCAN
};

struct rpu_display_scan_params {
#define MAX_SCAN_RESULT 100
	struct  umac_display_results *res[MAX_SCAN_RESULT];
	int     num;
};

struct rpu_connect_params {
	unsigned char  ssid[MAX_SSID_LEN];
	unsigned char   bssid[NRF_WIFI_ETH_ADDR_LEN];
	unsigned int    freq;
	unsigned int   ssid_len;
	unsigned char  channel;
	enum connect_status status;
	enum scan_type  scan_type;
};
#endif /*CMD_DEMO*/


struct nrf_wifi_ctx_lnx {
	void *rpu_ctx;
#ifdef WLAN_SUPPORT
#ifdef RPU_CONFIG_FMAC
	struct nrf_wifi_fmac_vif_ctx_lnx *def_vif_ctx;
#ifdef HOST_CFG80211_SUPPORT	
	struct wiphy *wiphy;
#else
	unsigned int idx;
	struct device dev;
#endif /* HOST_CFG80211_SUPPORT */	
#endif /* RPU_CONFIG_FMAC */

	struct dentry *dbgfs_rpu_root;	
	struct dentry *dbgfs_wlan_root;
#ifdef CONF_SUPPORT
	struct dentry *dbgfs_wlan_conf_root;
	struct rpu_conf_params conf_params;
#endif /* CONF_SUPPORT */
#ifdef CMD_DEMO
	struct dentry *dbgfs_nrf_wifi_connect_root;
	struct rpu_display_scan_params *display_scan_params;
	struct rpu_connect_params connect_params;
#endif /*CMD_DEMO*/
	struct dentry *dbgfs_wlan_stats_root;
#ifdef DEBUG_MODE_SUPPORT
	struct nrf_wifi_umac_set_beacon_info info;
	struct rpu_btcoex btcoex;
#endif
	struct list_head cookie_list;	
#endif /* WLAN_SUPPORT */	
#ifdef TWT_SUPPORT
	struct dentry *dbgfs_nrf_wifi_twt_root;
	struct rpu_twt_params twt_params;
#endif
	
};


struct nrf_wifi_drv_priv_lnx {
	struct dentry *dbgfs_root;
	struct dentry *dbgfs_ver_root;
#ifdef WLAN_SUPPORT
	struct nrf_wifi_fmac_priv *fmac_priv;
#endif /* WLAN_SUPPORT */	
#ifndef HOST_CFG80211_SUPPORT
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx[MAX_NUM_RPU];
	unsigned char num_rpu;
#endif /* !HOST_CFG80211_SUPPORT */
	bool drv_init;
};


struct nrf_wifi_ctx_lnx *nrf_wifi_fmac_dev_add_lnx(void);
void nrf_wifi_fmac_dev_rem_lnx(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
enum nrf_wifi_status nrf_wifi_fmac_dev_init_lnx(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_fmac_dev_deinit_lnx(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
#endif /* __LNX_MAIN_H__ */
