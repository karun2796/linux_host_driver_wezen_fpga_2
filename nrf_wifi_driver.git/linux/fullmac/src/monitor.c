#ifdef HACK_MONITOR_MODE
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/ieee80211_radiotap.h>
#include "driver_linux.h"

#define printhex(d, l) \
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16, 1, d, l, 1)

struct hwsim_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	__le64 rt_tsft;
	u8 rt_flags;
	u8 rt_rate;
	__le16 rt_channel;
	__le16 rt_chbitmask;
} __packed;

int monitor_rx(struct sk_buff *skb)
{

	struct hwsim_radiotap_hdr *hdr;
	u16 flags;
	struct net_device *hwsim_mon = dev_get_by_name(&init_net, "hwsim0");

	if (!hwsim_mon)
		return 0;

	if (!netif_running(hwsim_mon))
		return 0;

	printhex(skb->data, skb->len);

	pr_err("hwsim0 interface found\n");

	skb = skb_copy_expand(skb, sizeof(*hdr), 0, GFP_ATOMIC);

	if (skb == NULL)
		return 1;

	hdr = (struct hwsim_radiotap_hdr *) skb_push(skb, sizeof(*hdr));
	hdr->hdr.it_version = PKTHDR_RADIOTAP_VERSION;
	hdr->hdr.it_pad = 0;
	hdr->hdr.it_len = cpu_to_le16(sizeof(*hdr));
	hdr->hdr.it_present = cpu_to_le32((1 << IEEE80211_RADIOTAP_FLAGS) |
					  (1 << IEEE80211_RADIOTAP_RATE) |
					  (1 << IEEE80211_RADIOTAP_TSFT) |
					  (1 << IEEE80211_RADIOTAP_CHANNEL));

	/*TODO Hard coded now made it as dynamic*/
	hdr->rt_tsft =  100;
	hdr->rt_flags = 0;
	hdr->rt_rate = 1;
	hdr->rt_channel = 108;
	flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM;
	hdr->rt_chbitmask = cpu_to_le16(flags);

	pr_err("hwsim0 interface running\n");

	skb->dev = hwsim_mon;
	skb_set_mac_header(skb, 0);
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_802_2);

	memset(skb->cb, 0, sizeof(skb->cb));

	pr_err("hwsim0 send the packet\n");

	netif_rx(skb);

	return 1;
}
#endif
