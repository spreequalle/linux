#include <linux/config.h>
#include <linux/types.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <net/checksum.h>
#include <net/ip.h>
#include <linux/stddef.h>
#include <linux/sysctl.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <linux/err.h>
#include <linux/percpu.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/tcp.h>

/* nf_conntrack_lock protects the main hash table, protocol/helper/expected
   registrations, conntrack timers*/
#define ASSERT_READ_LOCK(x)
#define ASSERT_WRITE_LOCK(x)

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/listhelp.h>
#include <linux/netfilter_ipv4/ipt_PORTTRIGGER.h>



MODULE_LICENSE("GPL");


#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

#define ASSERT_READ_LOCK(x) //MUST_BE_READ_LOCKED(&nf_conntrack_lock)
#define ASSERT_WRITE_LOCK(x) //MUST_BE_WRITE_LOCKED(&nf_conntrack_lock)
#include <linux/netfilter_ipv4/listhelp.h>

static struct ipt_porttrigger {
	struct list_head list;		
	struct timer_list timeout;	
	unsigned int src_ip;		
	unsigned int dst_ip;		
	unsigned short trigger_proto;
	unsigned short forward_proto;
	unsigned int timer;
	struct ipt_mport trigger_ports;
	struct ipt_mport forward_ports;
	struct nf_nat_range range;
};

static LIST_HEAD(trigger_list);

static unsigned int
del_porttrigger_rule(struct ipt_porttrigger *trigger)
{
	NF_CT_ASSERT(trigger);
	write_lock_bh(&nf_conntrack_lock);
	DEBUGP("del rule src_ip=%x,proto=%x,dst_ip=%x,proto=%x\n",trigger->src_ip,trigger->trigger_proto,trigger->dst_ip,trigger->forward_proto);
	list_del(&trigger->list);
	write_unlock_bh(&nf_conntrack_lock);
	kfree(trigger);
	return 0;
}


static void 
refresh_timer(struct ipt_porttrigger *trigger, unsigned long extra_jiffies)
{
	NF_CT_ASSERT(trigger);
	write_lock_bh(&nf_conntrack_lock);

	if(extra_jiffies == 0)
		extra_jiffies = TRIGGER_TIMEOUT * HZ;

	if (del_timer(&trigger->timeout)) {
		trigger->timeout.expires = jiffies + extra_jiffies;
		add_timer(&trigger->timeout);
	}
	write_unlock_bh(&nf_conntrack_lock);
}

static void timer_timeout(unsigned long in_trigger)
{
	struct ipt_porttrigger *trigger= (void *) in_trigger;
	write_lock_bh(&nf_conntrack_lock);
	del_porttrigger_rule(trigger);
	write_unlock_bh(&nf_conntrack_lock);
	DEBUGP("timer out, del trigger rule\n");
}


static inline int
ports_match(const struct ipt_mport *minfo, u_int16_t port)
{
	unsigned int i, m;
	u_int16_t s, e;
	u_int16_t pflags = minfo->pflags;
	
	for (i=0, m=1; i<IPT_MULTI_PORTS; i++, m<<=1) {
		if (pflags & m  && minfo->ports[i] == 65535){
			DEBUGP("port%x don't match=%d\n",port,i);
			return 0;
		}	

		s = minfo->ports[i];
		if (pflags & m) {
			e = minfo->ports[++i];
			m <<= 1;
		} else
			e = s;

		if ( port >= s && port <= e){ 
			//DEBUGP("s=%x,e=%x\n",s,e);
			return 1;
		}	
	}
	DEBUGP("ports=%x don't match\n",port);
	return 0;
}


static inline int 
packet_in_match(const struct ipt_porttrigger *trigger,
	const unsigned short proto, 
	const unsigned short dport,
	const unsigned int src_ip)
{
	/* 
	  Modification: for protocol type==all(any) can't work
      Modified by: ken_chiang 
      Date:2007/8/21
    */
#if 0	
	u_int16_t forward_proto = trigger->forward_proto;
	
	if (!forward_proto)
		forward_proto = proto;
	return ( (forward_proto == proto) && (ports_match(&trigger->forward_ports, dport)) );
#else
	u_int16_t forward_proto = trigger->forward_proto;
	DEBUGP("src_ip=%x,trigger->src_ip=%x in match\n",src_ip,trigger->src_ip);
	/* 
	  Modification: for trigge port==incomeing port can't work
      Modified by: ken_chiang 
      Date:2007/9/7
    */
	if(src_ip==trigger->src_ip){
		return 0;
	}	
	DEBUGP("proto=%x,dport=%x in match\n",proto,dport);
	if (!forward_proto){
		DEBUGP("forward_proto=null\n");
		return ( ports_match(&trigger->forward_ports, dport) );
	}
	else{
		DEBUGP("forward_proto=%x\n",forward_proto);
		return ( (trigger->forward_proto == proto) && (ports_match(&trigger->forward_ports, dport)) );
	}	
#endif
}

static inline int 
packet_out_match(const struct ipt_porttrigger *trigger,
	const unsigned short proto, 
	unsigned short dport)
{
	/* 
	  Modification: for protocol type==all(any) can't work
      Modified by: ken_chiang 
      Date:2007/8/21
    */
    u_int16_t trigger_proto = trigger->trigger_proto;
    DEBUGP("proto=%x,dport=%x out match\n",proto,dport);
	if (!trigger_proto){
		DEBUGP("trigger_proto=null\n");
		return ( ports_match(&trigger->trigger_ports, dport) );
	}	
	else{
		DEBUGP("trigger_proto=%x\n",trigger_proto);
		return ( (trigger->trigger_proto == proto) && (ports_match(&trigger->trigger_ports, dport)) );
	}	
}


static unsigned int
add_porttrigger_rule(struct ipt_porttrigger *trigger)
{
	struct ipt_porttrigger *rule;

	write_lock_bh(&nf_conntrack_lock);
	rule = (struct ipt_porttrigger *)kmalloc(sizeof(struct ipt_porttrigger), GFP_ATOMIC);

	if (!rule) {
		write_unlock_bh(&nf_conntrack_lock);
		return -ENOMEM;
	}

	memset(rule, 0, sizeof(*trigger));
	INIT_LIST_HEAD(&rule->list);
	memcpy(rule, trigger, sizeof(*trigger));
	DEBUGP("add rule src_ip=%x,proto=%x,dst_ip=%x,proto=%x\n\n\n",rule->src_ip,rule->trigger_proto,rule->dst_ip,rule->forward_proto);
	list_prepend(&trigger_list, &rule->list);
	init_timer(&rule->timeout);
	rule->timeout.data = (unsigned long)rule;
	rule->timeout.function = timer_timeout;
	DEBUGP("rule->timer=%x\n",rule->timer);
	DEBUGP("rule->src_ip=%x\n",rule->src_ip);
	/* 
	  Modification: for protocol type==all(any) sometime can't work if timer = 0
      Modified by: ken_chiang 
      Date:2007/8/31
    */
	if(rule->timer<600)
		rule->timer =600;
	DEBUGP("rule->timer2=%x\n",rule->timer);	
	rule->timeout.expires = jiffies + (rule->timer * HZ);
	add_timer(&rule->timeout);
	write_unlock_bh(&nf_conntrack_lock);
	return 0;
}


static unsigned int
porttrigger_nat(struct sk_buff **pskb,
		const struct net_device *in,
		const struct net_device *out,
		unsigned int hooknum,
		const void *targinfo)
{
	struct ip_conntrack *ct;
	enum ip_conntrack_info ctinfo;
	const struct iphdr *iph = (*pskb)->nh.iph;
	struct tcphdr *tcph = (void *)iph + iph->ihl*4;
	struct nf_nat_range newrange;
	struct ipt_porttrigger *found;

	NF_CT_ASSERT(hooknum == NF_IP_PRE_ROUTING);
	/* 
	  Modification: for trigge port==incomeing port can't work
      Modified by: ken_chiang 
      Date:2007/9/7
    */
	found = LIST_FIND(&trigger_list, packet_in_match, struct ipt_porttrigger *, iph->protocol, ntohs(tcph->dest),iph->saddr);
	if( !found || !found->src_ip ){
		//DEBUGP("DNAT: no find\n");
		return XT_CONTINUE;
	}	

	DEBUGP("DNAT: src IP %u.%u.%u.%u\n", NIPQUAD(found->src_ip));
	ct = nf_ct_get(*pskb, &ctinfo);
	newrange = ((struct nf_nat_range)
		{ IP_NAT_RANGE_MAP_IPS, found->src_ip, found->src_ip, 
		found->range.min, found->range.max });

	return nf_nat_setup_info(ct, &newrange, hooknum); 
}


static unsigned int
porttrigger_forward(struct sk_buff **pskb,
		  unsigned int hooknum,
		  const struct net_device *in,
		  const struct net_device *out,
		  const void *targinfo,
		  int mode)
{
	const struct ipt_porttrigger_info *info = targinfo;
	const struct iphdr *iph = (*pskb)->nh.iph;
	struct tcphdr *tcph = (void *)iph + iph->ihl*4;
	struct ipt_porttrigger trigger, *found, match;

	switch(mode)
	{
		case MODE_FORWARD_IN:
			/* 
	  			Modification: for trigge port==incomeing port can't work
      			Modified by: ken_chiang 
      			Date:2007/9/7
    		*/
			found = LIST_FIND(&trigger_list, packet_in_match, struct ipt_porttrigger *, iph->protocol, ntohs(tcph->dest),iph->saddr);
			if (found) {
				refresh_timer(found, info->timer * HZ);
				DEBUGP("FORWARD_IN found\n");		
				return NF_ACCEPT;
			}
			DEBUGP("FORWARD_IN no found\n");
			break;

		/* MODE_FORWARD_OUT */
		case MODE_FORWARD_OUT:
			found = LIST_FIND(&trigger_list, packet_out_match, struct ipt_porttrigger *, iph->protocol, ntohs(tcph->dest));
			if (found) {
				refresh_timer(found, info->timer * HZ);
				found->src_ip = iph->saddr;
				DEBUGP("FORWARD_OUT found ip=%x\n",found->src_ip);
			} else {
				DEBUGP("FORWARD_OUT no found\n");
				//memcpy(&match.trigger_ports, &info->trigger_ports, sizeof(struct ipt_mport));
				match.trigger_ports = info->trigger_ports;
				match.trigger_proto = info->trigger_proto;
					
				if( packet_out_match(&match, iph->protocol, ntohs(tcph->dest)) ) {
					DEBUGP("FORWARD_OUT_MATCH\n");
					memset(&trigger, 0, sizeof(trigger));
					trigger.src_ip = iph->saddr;
					DEBUGP("FORWARD_OUT trigger ip=%x\n",trigger.src_ip);
					trigger.trigger_proto = iph->protocol;					
					trigger.forward_proto = info->forward_proto;
					memcpy(&trigger.trigger_ports, &info->trigger_ports, sizeof(struct ipt_mport));
					memcpy(&trigger.forward_ports, &info->forward_ports, sizeof(struct ipt_mport));
					add_porttrigger_rule(&trigger);
				}

			}
			break;
	}

	return XT_CONTINUE;
}



unsigned int
porttrigger_target(struct sk_buff **pskb,
		const struct net_device *in,
		const struct net_device *out,
		unsigned int hooknum,
		const struct xt_target *target,
		const void *targinfo)
{
	const struct ipt_porttrigger_info *info = targinfo;
	const struct iphdr *iph = (*pskb)->nh.iph;

	if ((iph->protocol != IPPROTO_TCP) && (iph->protocol != IPPROTO_UDP))
		return XT_CONTINUE;

	if (info->mode == MODE_DNAT)
		return porttrigger_nat(pskb, in, out, hooknum, targinfo);
	else if (info->mode == MODE_FORWARD_OUT)
		return porttrigger_forward(pskb, hooknum, in, out, targinfo, MODE_FORWARD_OUT);
	else if (info->mode == MODE_FORWARD_IN)
		return porttrigger_forward(pskb, hooknum, in, out, targinfo, MODE_FORWARD_IN);

	return XT_CONTINUE;
}


static int
porttrigger_check(const char *tablename,
	      const void *entry,
	      const struct xt_target *target,
         void *targinfo,
	      unsigned int hook_mask){
	
	const struct ipt_entry *e = entry;
	const struct ipt_porttrigger_info *info = targinfo;
	struct list_head *cur, *tmp;

	if( info->mode == MODE_DNAT && strcmp(tablename, "nat") != 0) {
		DEBUGP("porttrigger_check: bad table `%s'.\n", tablename);
		return 0;
	}
	
	/*	remove check size, due to causing a error
	if (targinfosize != IPT_ALIGN(sizeof(*info))) {
		DEBUGP("porttrigger_check: size %u != %u.\n",
		       targinfosize, sizeof(*info));
		return 0;
	}
	*/
	
	if (hook_mask & ~((1 << NF_IP_PRE_ROUTING) | (1 << NF_IP_FORWARD))) {
		DEBUGP("porttrigger_check: bad hooks %x.\n", hook_mask);
		return 0;
	}
	if ( info->forward_proto != IPPROTO_TCP && info->forward_proto != IPPROTO_UDP && info->forward_proto != 0) {
		DEBUGP("porttrigger_check: bad trigger proto.\n");
		return 0;
	}

	list_for_each_safe(cur, tmp, &trigger_list) {
		struct ipt_porttrigger *trigger = (void *)cur;
		del_timer(&trigger->timeout);
		del_porttrigger_rule(trigger);
	}

	return 1;
}

static struct xt_target porttrigger = {
	.name 		= "PORTTRIGGER",
	.family		= AF_INET,
	.target 	= porttrigger_target,
	.targetsize	= sizeof(struct ipt_porttrigger_info),
//	.table		= "nat",
	.checkentry 	= porttrigger_check,
	.me 		= THIS_MODULE,
};

static int __init init(void)
{
	return xt_register_target(&porttrigger);
}

static void __exit fini(void)
{
	xt_unregister_target(&porttrigger);
}

module_init(init);
module_exit(fini);

