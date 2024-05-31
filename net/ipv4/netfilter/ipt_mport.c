/* Kernel module to match one of a list of TCP/UDP ports: ports are in
   the same place so we can treat them as equal. */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
//test
#include <linux/version.h>//test

#include <linux/netfilter_ipv4/ipt_mport.h>
#include <linux/netfilter_ipv4/ip_tables.h>

MODULE_LICENSE("GPL");

#if 0
#define duprintf(format, args...) printk(format , ## args)
#else
#define duprintf(format, args...)
#endif

/* Returns 1 if the port is matched by the test, 0 otherwise. */
static inline int
ports_match(const struct ipt_mport *minfo, u_int16_t src, u_int16_t dst)
{
	unsigned int i;
        unsigned int m;
        u_int16_t pflags = minfo->pflags;
	for (i=0, m=1; i<IPT_MULTI_PORTS; i++, m<<=1) {
                u_int16_t s, e;

                if (pflags & m
                    && minfo->ports[i] == 65535)
                        return 0;

                s = minfo->ports[i];

                if (pflags & m) {
                        e = minfo->ports[++i];
                        m <<= 1;
                } else
                        e = s;

                if (minfo->flags & IPT_MPORT_SOURCE
                    && src >= s && src <= e)
                        return 1;

		if (minfo->flags & IPT_MPORT_DESTINATION
		    && dst >= s && dst <= e)
			return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
////--ori code--////

//static int
//match(const struct sk_buff *skb,
//      const struct net_device *in,
//      const struct net_device *out,
//      const void *matchinfo,
//      int offset,
//      int *hotdrop)
//{
//	u16 ports[2];
//	const struct ipt_mport *minfo = matchinfo;
//
//	if (offset)
//		return 0;
//
//	/* Must be big enough to read ports (both UDP and TCP have
//           them at the start). */
//	if (skb_copy_bits(skb, skb->nh.iph->ihl*4, ports, sizeof(ports)) < 0) {
//		/* We've been asked to examine this packet, and we
//		   can't.  Hence, no choice but to drop. */
//			duprintf("ipt_multiport:"
//				 " Dropping evil offset=0 tinygram.\n");
//			*hotdrop = 1;
//			return 0;
//	}
//
//	return ports_match(minfo, ntohs(ports[0]), ntohs(ports[1]));
//}

//////////////////////////////////////////////////////////////////////

static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const struct xt_match *match,
      const void *matchinfo,
      int offset,
      unsigned int protoff,
      int *hotdrop)
{
	u16 ports[2];
	const struct ipt_mport *minfo = matchinfo;

	if (offset)
		return 0;

	/* Must be big enough to read ports (both UDP and TCP have
           them at the start). */
	if (skb_copy_bits(skb, skb->nh.iph->ihl*4, ports, sizeof(ports)) < 0) {
		/* We've been asked to examine this packet, and we
		   can't.  Hence, no choice but to drop. */
			duprintf("ipt_multiport:"
				 " Dropping evil offset=0 tinygram.\n");
			*hotdrop = 1;
			return 0;
	}

	return ports_match(minfo, ntohs(ports[0]), ntohs(ports[1]));
}

/* Called when user tries to insert an entry of this type. */

//////////////////////////////////////////////////////////////////////
////--ori checkentry code--////

//static int
//checkentry(const char *tablename,
//	   const struct ipt_ip *ip,
//	   void *matchinfo,
//	   unsigned int matchsize,
//	   unsigned int hook_mask)
//{
//	if (matchsize != IPT_ALIGN(sizeof(struct ipt_mport)))
//		return 0;
//
//	/* Must specify proto == TCP/UDP, no unknown flags or bad count */
//	return (ip->proto == IPPROTO_TCP || ip->proto == IPPROTO_UDP)
//		&& !(ip->invflags & IPT_INV_PROTO)
//		&& matchsize == IPT_ALIGN(sizeof(struct ipt_mport));
//}

//////////////////////////////////////////////////////////////////////

static int
checkentry(const char *tablename,
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
//           const void *ip,
//#else
           const struct ipt_ip *ip,
//#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
            const struct xt_match *match,
#endif
	   void *matchinfo,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
           unsigned int matchsize,
#endif
	   unsigned int hook_mask)
{
	const struct ipt_mport *info = matchinfo;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17)
	if (matchsize != IPT_ALIGN(sizeof(struct ipt_mport)))
		return 0;
#endif

	/* Must specify proto == TCP/UDP, no unknown flags or bad count */

	if ( !( (ip->proto == IPPROTO_TCP || ip->proto == IPPROTO_UDP)
		&& !(ip->invflags & IPT_INV_PROTO) ) )
		return 0;
	
	return 1;	
}



//////////////////////////////////////////////////////////////////////
////--ori code--////


//static struct ipt_match mport_match = { 
//	.name = "mport",
//	.match = &match,
//	.checkentry = &checkentry,
//	.me = THIS_MODULE
//};

//////////////////////////////////////////////////////////////////////


//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
static struct xt_match mport_match = {
//#else
//static struct ipt_match mport_match = {
//#endif
	.name		= "mport",
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	.family		= AF_INET,
//#endif
	.match		= &match,
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
	.matchsize      = sizeof(struct ipt_mport),
//#endif
	.checkentry	= &checkentry,
	.me = THIS_MODULE
};



//////////////////////////////////////////////////////////////////////
////--ori code--////

//static int __init init(void)
//{
//	return ipt_register_match(&mport_match);
//}

//////////////////////////////////////////////////////////////////////

static int __init init(void)
{
	//return ipt_register_match(&mport_match);
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	return xt_register_match(&mport_match);
//#else
//	return ipt_register_match(&mport_match);
//#endif
	
}

//////////////////////////////////////////////////////////////////////
////--ori code--////

//static void __exit fini(void)
//{
//	ipt_unregister_match(&mport_match);
//}

//////////////////////////////////////////////////////////////////////

static void __exit fini(void)
{
	//ipt_unregister_match(&mport_match);
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	xt_unregister_match(&mport_match);
//#else
//	ipt_unregister_match(&mport_match);
//#endif
	
}

module_init(init);
module_exit(fini);

