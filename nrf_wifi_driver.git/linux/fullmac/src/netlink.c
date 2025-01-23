#ifndef HOST_CFG80211_SUPPORT
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "netlink.h"
#include "fmac_api.h"

#define NETLINK_USER_1 31
#define NETLINK_USER_2 30

struct netlink_priv npriv;

int netlink_send(unsigned char *data,
		 int len,
		 NL_SOCK_TYPE sock_type)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct sock *send_sock = NULL;
	int ret = -1;

	skb = nlmsg_new(len, 0);

	if(!skb) {
		printk(KERN_ERR "Failed to allocate new skb\n");
		goto out;
	} 

	nlh = nlmsg_put(skb,
			0,
			0,
			NLMSG_DONE,
			len,
			0);  

	NETLINK_CB(skb).dst_group = 0; /* not in mcast group */

	memcpy(nlmsg_data(nlh), data, len);

	//printk(KERN_DEBUG "%s: Received event with length=%d\n",  __func__, len);

	if (sock_type == NL_SOCK_TYPE_ASYNC)
		send_sock = npriv.sk_async;
	else if (sock_type == NL_SOCK_TYPE_SYNC)
		send_sock = npriv.sk_sync;
	else {
		pr_err("%s: Invalid netlink socket type\n", __func__);
		goto out;
	}

	if (!send_sock) {
		printk(KERN_DEBUG "%s: Netlink socket not created\n",  __func__);
		goto out;
	}

	if (npriv.pid == 0) {
		printk(KERN_DEBUG "%s: Application does not exist, returning\n", __func__);
		ret = 0;
		goto out;
	}

	if (nlmsg_unicast(send_sock,
			  skb,
			  npriv.pid) < 0 ) {
		printk(KERN_DEBUG "%s: Failed to  send sync msg_unicast to pid=%d\n",  __func__, npriv.pid);
		goto out;
	}

#ifdef notyet
	if (sock_type == NL_SOCK_TYPE_SYNC)
		pr_err("Sent synchronous event successfully\n");
	else
		pr_err("Sent asynchronous event successfully\n");
#endif /* notyet */

	ret = 0;
out:
	return ret;
}


static void netlink_recv(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;

	nlh = (struct nlmsghdr*)skb->data;
	npriv.pid = nlh->nlmsg_pid;

	npriv.rcv_handler(nlmsg_data(nlh),
			  nlmsg_len(nlh));
}


void netlink_unbind(struct net *net, int group)
{
	pr_err("Netlink Unbind called\n");

	if (npriv.pid != 0) {
		printk(KERN_DEBUG "%s: Application with pid = %d Exited\n", __func__, npriv.pid);
		npriv.pid = 0;
	}
}


int netlink_init(void (*rcv_handler)(void *data,
				     unsigned int data_len))
{
	int ret = -1;

	/* This is for 3.6 kernels and above. */
	struct netlink_kernel_cfg cfg = {
		.groups = 1,
		.input = netlink_recv,
		.unbind = netlink_unbind,
	};

	npriv.rcv_handler = rcv_handler;

	npriv.sk_sync = netlink_kernel_create(&init_net, NETLINK_USER_1, &cfg);

	if(!npriv.sk_sync) {
		printk(KERN_ALERT "Error creating SYNC socket.\n");
		goto out;
	}

	npriv.sk_async = netlink_kernel_create(&init_net, NETLINK_USER_2, NULL);

	if(!npriv.sk_async) {
		printk(KERN_ALERT "Error creating ASYNC socket.\n");
		goto out;
	}

	ret = 0;
out:
	return ret;
}


void netlink_deinit(void)
{
	netlink_kernel_release(npriv.sk_sync);
	netlink_kernel_release(npriv.sk_async);
	npriv.rcv_handler = NULL;
}
#endif
