#ifndef HOST_CFG80211_SUPPORT
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef __NETLINK_H__
#define __NETLINK_H__

#include <linux/printk.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

typedef enum nl_sock_type {
	NL_SOCK_TYPE_ASYNC,
	NL_SOCK_TYPE_SYNC,
} NL_SOCK_TYPE;

struct netlink_priv {
	struct sock *sk_sync;
	struct sock *sk_async;
	int pid;

	void (*rcv_handler)(void *data,
			    unsigned int data_len);
};

int netlink_send(unsigned char *data,
	       	 int len,
		 NL_SOCK_TYPE sock_type);

int netlink_init(void (*rcv_handler)(void *data,
				     unsigned int data_len));

void netlink_deinit(void);
#endif /* __NETLINK_H__ */
#endif /* !HOST_CFG80211_SUPPORT */
