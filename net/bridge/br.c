/*
 *	Generic parts
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br.c,v 1.1.1.1 2007-05-25 06:50:00 bruce Exp $
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

//add by Allen
#ifdef DNS_HIJACK
#include <linux/proc_fs.h>
#endif
//end by Allen
#include "br_private.h"

//add by Allen
#ifdef DNS_HIJACK
long blankstatus =0;

static int proc_write_blankstatus(struct file *file, const char *buffer, unsigned long count, void *data){
	char *temp;
	temp = (char *)buffer;
	blankstatus = simple_strtol(temp, 0, 10);
	
	return count;
}

void create_blankstatus_proc(){
	static struct proc_dir_entry *blankstatus_proc;
	
	blankstatus_proc = create_proc_entry("blankstatus", 0, 0);
	
	if(blankstatus_proc){
		blankstatus_proc->write_proc = (write_proc_t*)&proc_write_blankstatus;
	}
}
#endif
//end by Allen

int (*br_should_route_hook) (struct sk_buff **pskb) = NULL;

static struct llc_sap *br_stp_sap;

static int __init br_init(void)
{
	int err;

	br_stp_sap = llc_sap_open(LLC_SAP_BSPAN, br_stp_rcv);
	if (!br_stp_sap) {
		printk(KERN_ERR "bridge: can't register sap for STP\n");
		return -EADDRINUSE;
	}

	br_fdb_init();

	err = br_netfilter_init();
	if (err)
		goto err_out1;

	err = register_netdevice_notifier(&br_device_notifier);
	if (err)
		goto err_out2;

	br_netlink_init();
	brioctl_set(br_ioctl_deviceless_stub);
	br_handle_frame_hook = br_handle_frame;

	br_fdb_get_hook = br_fdb_get;
	br_fdb_put_hook = br_fdb_put;

	//add by Allen
        #ifdef DNS_HIJACK
	create_blankstatus_proc();
        #endif
        //end by Allen

	return 0;

err_out2:
	br_netfilter_fini();
err_out1:
	llc_sap_put(br_stp_sap);
	return err;
}

static void __exit br_deinit(void)
{
	rcu_assign_pointer(br_stp_sap->rcv_func, NULL);

	br_netlink_fini();
	br_netfilter_fini();
	unregister_netdevice_notifier(&br_device_notifier);
	brioctl_set(NULL);

	br_cleanup_bridges();

	synchronize_net();

	llc_sap_put(br_stp_sap);
	br_fdb_get_hook = NULL;
	br_fdb_put_hook = NULL;

	br_handle_frame_hook = NULL;
	br_fdb_fini();
}

EXPORT_SYMBOL(br_should_route_hook);

module_init(br_init)
module_exit(br_deinit)
MODULE_LICENSE("GPL");
MODULE_VERSION(BR_VERSION);
