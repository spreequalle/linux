#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/tcp.h> //add by vincent
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/inetdevice.h>
#include <net/protocol.h>
#include <net/checksum.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter/x_tables.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_tuple.h>
#include <linux/netfilter_ipv4/ip_autofw.h>//add
#include <linux/netfilter_ipv4/lockhelp.h>//add
#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter_ipv4/ipt_TRIGGER.h>

#define ASSERT_READ_LOCK(x) MUST_BE_READ_LOCKED(&nf_conntrack_lock)
#define ASSERT_WRITE_LOCK(x) MUST_BE_WRITE_LOCKED(&nf_conntrack_lock)
#include <linux/netfilter_ipv4/listhelp.h> //add

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif


struct ipt_trigger {
	struct list_head list;		/* Trigger list */
	struct timer_list timeout;	/* Timer for list destroying */
	u_int32_t srcip;		/* Outgoing source address */
	u_int32_t dstip;		/* Outgoing destination address */
	u_int16_t mproto;		/* Trigger protocol */
	u_int16_t rproto;		/* Related protocol */
	struct ipt_trigger_ports ports;	/* Trigger and related ports */
	u_int8_t reply;			/* Confirm a reply connection */
};


static unsigned int
trigger_out(struct sk_buff **pskb,
		unsigned int hooknum,
		const struct net_device *in,
		const struct net_device *out,
		const void *targinfo)
{

    const struct ipt_trigger_info *info = targinfo;
    struct ipt_trigger trig, *found;
    const struct iphdr *iph = (*pskb)->nh.iph;
    
    struct tcphdr *tcph = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */

    u_int16_t change_port=1900;
    tcph->dest=htons(change_port);
    u_int16_t change_checksum=ntohs(tcph->check)+1;
    tcph->check=htons(change_checksum);
 
    return NF_ACCEPT;
    
}

static unsigned int
trigger_target(struct sk_buff **pskb,
		const struct net_device *in,
		const struct net_device *out,
		unsigned int hooknum,
		const struct xt_target *target,
		const void *targinfo)

{
    const struct ipt_trigger_info *info = targinfo;
    const struct iphdr *iph = (*pskb)->nh.iph;
    
    /* Only supports TCP and UDP. */
    if ((iph->protocol != IPPROTO_TCP) && (iph->protocol != IPPROTO_UDP))
	return XT_CONTINUE;

    if (info->type == IPT_TRIGGER_OUT)
	return trigger_out(pskb, hooknum, in, out, targinfo);
    	
    return XT_CONTINUE;
}


static int
trigger_check(const char *tablename,
	      const void *entry,
	      const struct xt_target *target,
              void *targinfo,
	       unsigned int hook_mask)

{
	const struct ipt_entry *e = entry;
	const struct ipt_trigger_info *info = targinfo;
	struct list_head *cur_item, *tmp_item;
	
	if((strcmp(tablename, "mangle") == 0)) {
		DEBUGP("trigger_check: bad table `%s'.\n", tablename);
		//printk(KERN_EMERG "trigger_check: bad table %s\n",tablename);
		return 0;
	}
	
	if(info->proto) {
	    if (info->proto != IPPROTO_TCP && info->proto != IPPROTO_UDP) {
		DEBUGP("trigger_check: bad proto %d.\n", info->proto);
		//printk(KERN_EMERG "trigger_check: bad proto %d\n",info->proto);
		return 0;
	    }
	}
	
	if(info->type == IPT_TRIGGER_OUT) {
	    if (!info->ports.mport[0] || !info->ports.rport[0]) {
		DEBUGP("trigger_check: Try 'iptbles -j TRIGGER -h' for help.\n");
		//printk(KERN_EMERG "trigger_check: Try 'iptbles -j TRIGGER -h' for help.\n");
		return 0;
	    }
	}
	return 1;
}

static struct xt_target redirect_reg = {
         .name           = "TRIGGER", 
         .family	 = AF_INET,
         .target         = trigger_target,  
         .targetsize     = sizeof(struct ipt_trigger_info), 
         .checkentry     = trigger_check, 
         .me             = THIS_MODULE, 
}; 

static int __init init(void)
{
	//printk(KERN_EMERG "__init init");
	return xt_register_target(&redirect_reg);
}

static void __exit fini(void)
{
	//printk(KERN_EMERG "__exit fini");
	xt_unregister_target(&redirect_reg);
}

module_init(init);
module_exit(fini);
