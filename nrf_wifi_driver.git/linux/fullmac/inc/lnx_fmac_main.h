/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LNX_FMAC_MAIN_H__
#define __LNX_FMAC_MAIN_H__

#include <net/cfg80211.h>
#include "fmac_structs.h"
#include "sta.h"
#include "ap.h"
#include "p2p.h"
#include "host_rpu_umac_if.h"
#ifdef RPU_MODE_EXPLORER
#include "driver_linux.h"
#endif /* RPU_MODE_EXPLORER */

struct nrf_wifi_fmac_vif_ctx_lnx {
	struct nrf_wifi_ctx_lnx *rpu_ctx;
	struct net_device *netdev;
#ifdef HOST_CFG80211_SUPPORT	
	struct wireless_dev *wdev;
	struct cfg80211_bss *bss;
	struct cfg80211_scan_request *nrf_wifi_scan_req;
#endif /* HOST_CFG80211_SUPPORT */	

	unsigned char if_idx;

	/* event responses */
	struct nrf_wifi_sta_info *station_info;
	struct nrf_wifi_chan_definition *chan_def;
	int tx_power;
	int event_tx_power;
	int event_set_if;
	int status_set_if;
	struct p2p_info p2p;
	unsigned long rssi_record_timestamp_us;
        signed short rssi;
	char ifname[16];
};

struct nrf_wifi_fmac_vif_ctx_lnx *nrf_wifi_lnx_wlan_fmac_add_vif(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx,
								const char *name,
								char *mac_addr,
								enum nl80211_iftype if_type);
void nrf_wifi_lnx_wlan_fmac_del_vif(struct nrf_wifi_fmac_vif_ctx_lnx *vif_ctx_lnx);
#endif /* __LNX_FMAC_MAIN_H__ */
