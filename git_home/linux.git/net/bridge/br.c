/*
 *	Generic parts
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/llc.h>
#include <net/llc.h>
#include <net/stp.h>

#include "br_private.h"

#ifdef CONFIG_DNI_MCAST_TO_UNICAST
#include <linux/proc_fs.h>
struct __mac_cache mac_cache[MCAST_ENTRY_SIZE];
int igmp_snoop_enable = 0;
struct __mgroup_list *mhead = NULL;
static struct proc_dir_entry *mcast_proc_entry;
static struct proc_dir_entry *igmp_snoop_proc_entry;
static struct proc_dir_entry *mcast_set_proc_entry;
extern struct proc_dir_entry proc_root;
DEFINE_SPINLOCK(spin_igmp_lock);
#endif

#ifdef CONFIG_DNI_MCAST_TO_UNICAST
unsigned long
a2n(char *addr)
{
	int i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	sscanf(addr,"%d.%d.%d.%d", &i1, &i2, &i3, &i4);
	return (unsigned long)( (i1<<24) | (i2<<16) | (i3<<8) | (i4));
}

ssize_t mcast_read( struct file *filp, const char __user *buff,
											unsigned long len, void *data )
{
	struct __mgroup_list *ptr = mhead;
	struct __mgroup_mbr_list *mptr;
	int i;
	spin_lock_bh(&spin_igmp_lock);
	while (ptr)
	{
		printk("group %8x has wireless member :\n", ptr->gip);
		mptr = ptr->member;
		while (mptr)
		{
			printk("client : %8x\n", mptr->sip);
			mptr = mptr->next;
		}
		ptr = ptr->next;
	}
	for (i = 0; i < MCAST_ENTRY_SIZE; i++)
	{
		if (mac_cache[i].valid)
			printk("mac cache entry %d for %x (%2x:%2x:%2x:%2x:%2x:%2x)\n",
					i, mac_cache[i].sip, mac_cache[i].mac[0], mac_cache[i].mac[1], mac_cache[i].mac[2],
					mac_cache[i].mac[3], mac_cache[i].mac[4], mac_cache[i].mac[5]);
	}
	spin_unlock_bh(&spin_igmp_lock);
	return 0;
}

ssize_t mcast_write( struct file *filp, const char __user *buff,
							unsigned long len, void *data)
{
	char line[64];
	char *ptr, *tmp = line;
	unsigned long ip, gip;

	if (len >= 64) {
		printk("mcast_write: input buf len too long(%u)\n", len);
		return -EFAULT;
	}

	if (copy_from_user( line, buff, len ))
		return -EFAULT;
	
	line[len] = 0;

	//printk("mcast_write: input buf is %s, len: %u\n", line, len);

	ptr = strsep(&tmp, " ");
	switch(line[0])
	{
		case 'a':
		case 'd':
			ptr = strsep(&tmp, " ");
			ip = a2n(ptr);
			gip = a2n(tmp);
			proc_mcast_entry(line[0], ip, gip);
			break;
		default:
			break;
	}
	return len;
}

ssize_t igmp_snoop_read( struct file *filp, const char __user *buff,
								unsigned long len, void *data)
{
	printk("IGMP Snoop %s.\n", (igmp_snoop_enable)?"Enable":"Disable");
	return 0;
}

ssize_t igmp_snoop_write( struct file *filp, const char __user *buff,
									unsigned long len, void *data)
{
	char line[4];

	if (len >= 4) {
		printk("igmp_snoop_write: input buf len too long(%u)\n", len);
		return -EFAULT;
	}

	if (copy_from_user( line, buff, len ))
		return -EFAULT;
	
	line[len] = 0;

	//printk("igmp_snoop_write: input buf is %s, len: %u\n", line, len);

	igmp_snoop_enable = line[0] - '0';

	return len;
}

static const struct file_operations proc_mcast_operations = {
	.read       = mcast_read,
	.write      = mcast_write,
};

static const struct file_operations proc_igmp_snoop_operations = {
	.read       = igmp_snoop_read,
	.write      = igmp_snoop_write,
};

static const struct file_operations proc_mcast_set_operations = {
	.read       = mcast_set_read,
	.write      = mcast_set_write,
};

void create_porc_entry(void)
{
	igmp_snoop_enable = 0;
	mcast_proc_entry = proc_create(BR_PROC_MCAST_NAME, 0666, NULL,&proc_mcast_operations);

	igmp_snoop_proc_entry = proc_create(BR_IGMP_SNOOP_NAME, 0666, NULL,&proc_igmp_snoop_operations);

	memset(mac_cache, 0, MCAST_ENTRY_SIZE* sizeof(struct __mac_cache));

	mcast_set_proc_entry = proc_create(BR_MCAST_SET_NAME, 0666, NULL,&proc_mcast_set_operations);
}

void delete_porc_entry(void)
{
	igmp_snoop_enable = 0;
	//remove_proc_entry(BR_MCAST_SET_NAME, &proc_root);
	//remove_proc_entry(BR_IGMP_SNOOP_NAME, &proc_root);
	remove_proc_entry(BR_PROC_MCAST_NAME, &proc_root);
}
#endif

#ifdef CONFIG_DNI_DNSHIJACK_APMODE
struct sock *dnsnl = NULL;
#endif

static const struct stp_proto br_stp_proto = {
	.rcv	= br_stp_rcv,
};

static struct pernet_operations br_net_ops = {
	.exit	= br_net_exit,
};

static int __init br_init(void)
{
	int err;

	err = stp_proto_register(&br_stp_proto);
	if (err < 0) {
		pr_err("bridge: can't register sap for STP\n");
		return err;
	}

	err = br_fdb_init();
	if (err)
		goto err_out;

	err = register_pernet_subsys(&br_net_ops);
	if (err)
		goto err_out1;

	err = br_netfilter_init();
	if (err)
		goto err_out2;

	err = register_netdevice_notifier(&br_device_notifier);
	if (err)
		goto err_out3;

	err = br_netlink_init();
	if (err)
		goto err_out4;

	brioctl_set(br_ioctl_deviceless_stub);

#if IS_ENABLED(CONFIG_ATM_LANE)
	br_fdb_test_addr_hook = br_fdb_test_addr;
#endif

#ifdef CONFIG_DNI_MCAST_TO_UNICAST
	create_porc_entry();
#endif
#ifdef CONFIG_BRIDGE_NETGEAR_ACL
	br_acl_init();
#endif

#ifdef CONFIG_DNI_DNSHIJACK_APMODE
	struct netlink_kernel_cfg cfg = {
		.groups  = 0,
	};
	dnsnl = netlink_kernel_create(&init_net, NETLINK_NFLOG, &cfg);
	if (dnsnl == NULL)
		printk("Cannot create netlink socket: NETLINK_NFLOG\n");
#endif

	return 0;
err_out4:
	unregister_netdevice_notifier(&br_device_notifier);
err_out3:
	br_netfilter_fini();
err_out2:
	unregister_pernet_subsys(&br_net_ops);
err_out1:
	br_fdb_fini();
#ifdef CONFIG_DNI_MCAST_TO_UNICAST
	delete_porc_entry();
#endif
#ifdef CONFIG_BRIDGE_NETGEAR_ACL
	br_acl_fini();
#endif
#ifdef CONFIG_DNI_DNSHIJACK_APMODE
	if (dnsnl)
		netlink_kernel_release(dnsnl);
#endif

err_out:
	stp_proto_unregister(&br_stp_proto);
	return err;
}

static void __exit br_deinit(void)
{
	stp_proto_unregister(&br_stp_proto);

	br_netlink_fini();
	unregister_netdevice_notifier(&br_device_notifier);
	brioctl_set(NULL);

	unregister_pernet_subsys(&br_net_ops);

	rcu_barrier(); /* Wait for completion of call_rcu()'s */

	br_netfilter_fini();
#if IS_ENABLED(CONFIG_ATM_LANE)
	br_fdb_test_addr_hook = NULL;
#endif

	br_fdb_fini();
}

module_init(br_init)
module_exit(br_deinit)
MODULE_LICENSE("GPL");
MODULE_VERSION(BR_VERSION);
MODULE_ALIAS_RTNL_LINK("bridge");
