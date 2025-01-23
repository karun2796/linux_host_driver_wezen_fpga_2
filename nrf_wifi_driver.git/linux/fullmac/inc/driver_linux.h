#ifdef RPU_MODE_EXPLORER
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <linux/pci.h>
#include <net/cfg80211.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include "lnx_fmac_main.h"

#include "hal_api.h"
#include "fmac_api.h"

#include "vcu118.h"

#ifdef HACK_MONITOR_MODE
int monitor_rx(struct sk_buff *skb);
#endif
#endif /* RPU_MODE_EXPLORER */
