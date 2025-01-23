/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __LNX_NET_STACK_H__
#define __LNX_NET_STACK_H__

#include "lnx_main.h"
#include "lnx_fmac_main.h"

#ifndef CONFIG_NRF700X_RADIO_TEST

struct nrf_wifi_fmac_vif_ctx_lnx *nrf_wifi_netdev_add_vif(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx,
							      const char *if_name,
#ifdef HOST_CFG80211_SUPPORT
							      struct wireless_dev *wdev,
#endif /* HOST_CFG80211_SUPPORT */
							      char *mac_addr);

void nrf_wifi_netdev_del_vif(struct net_device *netdev);

void nrf_wifi_netdev_frame_rx_callbk_fn(void *vif_ctx,
					  void *frm);

enum nrf_wifi_status nrf_wifi_netdev_if_state_chg_callbk_fn(void *vif_ctx,
							    enum nrf_wifi_fmac_if_carr_state if_state);
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#endif /* __LNX_NET_STACK_H__ */
