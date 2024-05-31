/*
 *	Handle incoming frames
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br_input.c,v 1.1.1.1 2007-05-25 06:50:00 bruce Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/netfilter_bridge.h>
//add by Allen
//#ifdef DNS_HIJACK
//#include <linux/in.h>
//#include <linux/ip.h>
//#include <net/udp.h>, 
//#include <net/protocol.h>
//#include <net/inet_common.h>
//#include <linux/if_addr.h>
//#include <linux/if_ether.h>
//#include <linux/inet.h>
//#include <net/arp.h>
//#include <net/ip.h>
//#include <net/route.h>
//#include <net/ip_fib.h>
//#include <net/netlink.h>
//#include <linux/skbuff.h>
//#include <linux/rtnetlink.h>
//#include <linux/init.h>
//#include <linux/notifier.h>
//#include <linux/inetdevice.h>
//#include <linux/igmp.h>
//#include <linux/string.h>
//#include <linux/types.h>
//#include <linux/socket.h>
//#include <linux/delay.h>
//#include <linux/timer.h>
//#include <net/checksum.h>
//#endif //DNS_HIJACK
//end by Allen
#include "br_private.h"

//#ifdef DNS_HIJACK
//extern long blankstatus;
////int change_local_flag = 0;
//#endif DNS_HIJACK

/* Bridge group multicast address 802.1d (pg 51). */
const u8 br_group_address[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

static void br_pass_frame_up(struct net_bridge *br, struct sk_buff *skb)
{
	struct net_device *indev;

	br->statistics.rx_packets++;
	br->statistics.rx_bytes += skb->len;

	indev = skb->dev;
	skb->dev = br->dev;

	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_IN, skb, indev, NULL,
		netif_receive_skb);
}

/* note: already called with rcu_read_lock (preempt_disabled) */
int br_handle_frame_finish(struct sk_buff *skb)
{
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	struct net_bridge_port *p = rcu_dereference(skb->dev->br_port);
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
	int passedup = 0;

	if (!p || p->state == BR_STATE_DISABLED)
		goto drop;

	/* insert into forwarding database after filtering to avoid spoofing */
	br = p->br;
	br_fdb_update(br, p, eth_hdr(skb)->h_source);

	if (p->state == BR_STATE_LEARNING)
		goto drop;

	if (br->dev->flags & IFF_PROMISC) {
		struct sk_buff *skb2;

		skb2 = skb_clone(skb, GFP_ATOMIC);
		if (skb2 != NULL) {
			passedup = 1;
			br_pass_frame_up(br, skb2);
		}
	}

	if (is_multicast_ether_addr(dest)) {
		br->statistics.multicast++;
		br_flood_forward(br, skb, !passedup);
		if (!passedup)
			br_pass_frame_up(br, skb);
		goto out;
	}

	dst = __br_fdb_get(br, dest);
	if (dst != NULL && dst->is_local) {
		if (!passedup)
			br_pass_frame_up(br, skb);
		else
			kfree_skb(skb);
		goto out;
	}

	if (dst != NULL) {
		br_forward(dst->dst, skb);
		goto out;
	}

	br_flood_forward(br, skb, 0);

out:
	return 0;
drop:
	kfree_skb(skb);
	goto out;
}

/* note: already called with rcu_read_lock (preempt_disabled) */
static int br_handle_local_finish(struct sk_buff *skb)
{
	struct net_bridge_port *p = rcu_dereference(skb->dev->br_port);

	if (p && p->state != BR_STATE_DISABLED)
		br_fdb_update(p->br, p, eth_hdr(skb)->h_source);

	return 0;	 /* process further */
}

/* Does address match the link local multicast address.
 * 01:80:c2:00:00:0X
 */
static inline int is_link_local(const unsigned char *dest)
{
	return memcmp(dest, br_group_address, 5) == 0 && (dest[5] & 0xf0) == 0;
}

//#ifdef DNS_HIJACK
//#define ALLHEADER_LEN 27 //ETHHEADER_LEN 14, IPHEADER_LEN 20,UDPHEADER_LEN 8
//#define TWO_BYTES_FLAG 2
//#define ONE_BYTE_FLAG 1
//#define SHORT_DOMAIN_LEN 14
//#define LONG_DOMAIN_LEN 18
//#define ANSWERS_LEN 10
//
//const unsigned char* dns1content[] = 
//{ 0x03, 0x77, 0x77, 0x77,  //www
//	0x09, 0x6d, 0x79, 0x77, 0x69, 0x66, 0x69, 0x65, 0x78, 0x74, //mywifiext 
//	0x03, 0x6e, 0x65, 0x74, 0x00 };   //net
//const unsigned char* dns2content[] = 
//{ 0x03, 0x77, 0x77, 0x77,  //www
//	0x09, 0x6d, 0x79, 0x77, 0x69, 0x66, 0x69, 0x65, 0x78, 0x74, //mywifiext 
//	0x03, 0x63, 0x6f, 0x6d, 0x00 };   //com
//const unsigned char* hijackall[] = 
//{ 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, //transaction, flags, question, answer,authority, additional
//	0x09, 0x6d, 0x79, 0x77, 0x69, 0x66, 0x69, 0x65, 0x78, 0x74, 0x03, 0x6e, 0x65, 0x74, 0x00,//mywifiext.net 
//	0x00, 0x01, 0x00, 0x01, //type, class
//	0x09, 0x6d, 0x79, 0x77, 0x69, 0x66, 0x69, 0x65, 0x78, 0x74, 0x03, 0x6e, 0x65, 0x74, 0x00,//mywifiext.net 
//	0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 //type, class, ttl, data length
//};	
//const unsigned char* xboxdomain_like[] = 
//{ 0x08, 0x78, 0x62, 0x6f, 0x78, 0x6c, 0x69, 0x76, 0x65, //xboxlive
//	0x03, 0x63, 0x6f, 0x6d, 0x00 };//com
//	
//const unsigned char* xboxdomain_diff[] = 
//{ 0x02, 0x61, 0x73,  //as.
//	0x03, 0x74, 0x67, 0x73, //tgs.
//	0x04, 0x6d, 0x61, 0x63, 0x73 };//macs.
//
//void dns_hijack_src(struct sk_buff *skb)
//{
//	//struct sk_buff *skb = *pskb;	
//	struct iphdr *ip = skb->nh.iph;
//	//add by Allen
//	__u32 saddr=0, daddr=0;
//	__u16 src_port=0, dst_port=0;
//	int i;	
//	int change =0 ;
//	unsigned int udphoff =0 ;
//	int pos =0;
//	int dns_flags=0;	
//	int test_post1 =0, test_post2 = 0;
//	struct net_device *dev;
//	struct in_device *in_dev;
//	__u32 braddr;
//	struct in_ifaddr **ifap = NULL;
//	struct in_ifaddr *ifa = NULL;
//	
//	//add by Allen
//	if(ip->protocol == IPPROTO_UDP)
//	{
//		struct udphdr *uh=(struct udphdr *)((__u32 *)ip+ip->ihl);
//		src_port = ntohs(uh->source);
//		dst_port = ntohs(uh->dest);
//		udphoff = ip->ihl*4;	
//		
//		if(src_port == 0x35)
//		{
//			//printk("UDP %d -> src %u.%u.%u.%u:%u,dest %u.%u.%u.%u:%u\n",ip->protocol,NIPQUAD(ip->saddr),src_port,NIPQUAD(ip->daddr),dst_port);
//			//printk("************* blankstatus = %d\n",blankstatus);
//			//check blankstatus
//			if( blankstatus == 1) //in blank status
//			{
//				change =2 ;
//				//parse string
//				for(i=0;i<(skb->len);i++)
//		    {		   
//		      //printk("%x ",skb->data[i]);
//			    if(skb->data[i] == xboxdomain_like[0] && skb->data[i+1] == xboxdomain_like[1] && skb->data[i+2] == xboxdomain_like[2]
//		      && skb->data[i+3] == xboxdomain_like[3] && skb->data[i+4] == xboxdomain_like[4] && skb->data[i+5] == xboxdomain_like[5]
//		      && skb->data[i+6] == xboxdomain_like[6] && skb->data[i+7] == xboxdomain_like[7] && skb->data[i+8] == xboxdomain_like[8]
//		      && skb->data[i+9] == xboxdomain_like[9] && skb->data[i+10] == xboxdomain_like[10] && skb->data[i+11] == xboxdomain_like[11]
//		      && skb->data[i+12] == xboxdomain_like[12] 
//		      )
//		      {
//		      	if(skb->data[i-3] == xboxdomain_diff[0] && skb->data[i-2] == xboxdomain_diff[1]&& skb->data[i-1] == xboxdomain_diff[2]) 
//		   	    {
//		   	    	 //printk("\nGot the as.xboxlive.com\n");
//		   	     change =1;
//		   	     break;
//		   	    }
//		   	    if(skb->data[i-4] == xboxdomain_diff[3] && skb->data[i-3] == xboxdomain_diff[4] && skb->data[i-2] == xboxdomain_diff[5]&& skb->data[i-1] == xboxdomain_diff[6]) 
//		   	    {
//		   	    	 //printk("\nGot the tgs.xboxlive.com\n");
//		   	     change =1;
//		   	     break;
//		   	    }
//		   	    if(skb->data[i-5] == xboxdomain_diff[7] && skb->data[i-4] == xboxdomain_diff[8] && skb->data[i-3] == xboxdomain_diff[9] && skb->data[i-2] == xboxdomain_diff[10]&& skb->data[i-1] == xboxdomain_diff[11]) 
//		   	    {
//		   	    	 //printk("\nGot the macs.xboxlive.com\n");
//		   	     change =1;
//		   	     break;
//		   	    }
//		   	     
//		      }		      
//		    }
//			}
////			else if(blankstatus == 0)  // in none blank status
////			{
//				change = 1;
//		    //parse string
//		    for(i=0;i<(skb->len);i++)
//		    {		   
//		      //printk("%x ",skb->data[i]);
//			    if(skb->data[i] == dns2content[0] && skb->data[i+1] == dns2content[1] && skb->data[i+2] == dns2content[2]
//		      && skb->data[i+3] == dns2content[3] && skb->data[i+4] == dns2content[4] && skb->data[i+5] == dns2content[5]
//		      && skb->data[i+6] == dns2content[6] && skb->data[i+7] == dns2content[7] && skb->data[i+8] == dns2content[8]
//		      && skb->data[i+9] == dns2content[9] && skb->data[i+10] == dns2content[10] && skb->data[i+11] == dns2content[11]
//		      && skb->data[i+12] == dns2content[12] && skb->data[i+13] == dns2content[13] && skb->data[i+14] == dns2content[14]
//		      && skb->data[i+15] == dns2content[15] && skb->data[i+16] == dns2content[16] && skb->data[i+17] == dns2content[17]
//		      )
//		      {
//		   	     //printk("\nI got the www.mywifiext.com\n");
//		   	     change =2;
//		   	     break;
//		      }
//		      else if(skb->data[i] == dns2content[4] && skb->data[i+1] == dns2content[5]
//		      && skb->data[i+2] == dns2content[6] && skb->data[i+3] == dns2content[7] && skb->data[i+4] == dns2content[8]
//		      && skb->data[i+5] == dns2content[9] && skb->data[i+6] == dns2content[10] && skb->data[i+7] == dns2content[11]
//		      && skb->data[i+8] == dns2content[12] && skb->data[i+9] == dns2content[13] && skb->data[i+10] == dns2content[14]
//		      && skb->data[i+11] == dns2content[15] && skb->data[i+12] == dns2content[16] && skb->data[i+13] == dns2content[17]
//		      )		     
//		      {
//		        //printk("\nI got the mywifiext.com\n");
//		        change =2 ;
//		        break;
//		      }		   
//          if(skb->data[i] == dns1content[0] && skb->data[i+1] == dns1content[1] && skb->data[i+2] == dns1content[2]
//		      && skb->data[i+3] == dns1content[3] && skb->data[i+4] == dns1content[4] && skb->data[i+5] == dns1content[5]
//		      && skb->data[i+6] == dns1content[6] && skb->data[i+7] == dns1content[7] && skb->data[i+8] == dns1content[8]
//		      && skb->data[i+9] == dns1content[9] && skb->data[i+10] == dns1content[10] && skb->data[i+11] == dns1content[11]
//		      && skb->data[i+12] == dns1content[12] && skb->data[i+13] == dns1content[13] && skb->data[i+14] == dns1content[14]
//		      && skb->data[i+15] == dns1content[15] && skb->data[i+16] == dns1content[16] && skb->data[i+17] == dns1content[17]
//		      )			 
//			{
//		   	     //printk("\nI got the www.mywifiext.net\n");
//		   	     change =2;
//		   	     break;
//		  }
//		      if(skb->data[i] == dns1content[4] && skb->data[i+1] == dns1content[5]
//		      && skb->data[i+2] == dns1content[6] && skb->data[i+3] == dns1content[7] && skb->data[i+4] == dns1content[8]
//		      && skb->data[i+5] == dns1content[9] && skb->data[i+6] == dns1content[10] && skb->data[i+7] == dns1content[11]
//		      && skb->data[i+8] == dns1content[12] && skb->data[i+9] == dns1content[13] && skb->data[i+10] == dns1content[14]
//		      && skb->data[i+11] == dns1content[15] && skb->data[i+12] == dns1content[16] && skb->data[i+13] == dns1content[17]
//		      )
//			{
//		        //printk("\nI got the mywifiext.net\n");
//		        change =2 ;
//		        break;
//		      }		      
//		  }
////		  }
//		  //check change 
//		  if (change == 2 )
//		  {
//		  	//if( blankstatus == 1)
//		  	//printk("Need to do dns hijack\n");		  	 				
		  	//ip->tot_len = 0xa000; // 160
//		  	ip->check = 0x0000;		  	
//		  	//count the ip's checksum again
//		  	ip->check =(ip_fast_csum((u8 *)ip, ip->ihl));
//		  	//printk("ip header checksum is %lx\n",ip->check);
//		  	uh->len=0x8c00; //maybe has problem with IP header!! 0x8c00 = 0x8c = 140
//		  	//payload size
//		  	//printk("before small to big packet,The packet's len =%d,data_len=%d\n",skb->len,skb->data_len);
//		  	//printk("before small to big packet,The payload truezies =%d,datasize=%d\n",skb->truesize,(skb->truesize)-sizeof(struct sk_buff));
//		  	skb->truesize =4032;
//		  	skb->len = 160;
//		  	//printk("************ small packet to big packet,len =%d\n",skb->len);
//		  	//payload size
//		  	//printk("after small to big packet,The payload truezies =%d,datasize=%d\n",skb->truesize,(skb->truesize)-sizeof(struct sk_buff));
//		  	//printk("The payload truesizes =%d,datasize= %d\n",skb->truesize,(skb->truesize)-sizeof(struct sk_buff));
//		  	//get br0's ip
//		    dev = dev_get_by_name("br0");
//		  	in_dev = in_dev_get(dev);
//        for (ifap = &in_dev->ifa_list; (ifa = *ifap) != NULL;ifap = &ifa->ifa_next) 
//        {
//         	if(!strcmp(ifa->ifa_label,"br0"))
//          	 braddr = ifa->ifa_local;
//        }				
//		  	//printk("************* br0'ip : %u.%u.%u.%u\n",NIPQUAD(braddr));
//		  	//write data to packet		    
//		  	uh->check=0;
//		    skb->ip_summed = CHECKSUM_UNNECESSARY;
//		  	//printk("\nModify and Add DNS response\n");
//		  	dns_flags = ALLHEADER_LEN+TWO_BYTES_FLAG+ONE_BYTE_FLAG;
//		  	//zero all data - for blank state 
//		  	for(i=0;i<54;i++)
//		  	{
//		  		skb->data[dns_flags+i] = hijackall[i];
//		  	}
//		  	test_post2 = dns_flags+i;
//  			//printk("the old content =%x,%x,%x,%x ",skb->data[test_post2],skb->data[test_post2+1],skb->data[test_post2+2],skb->data[test_post2+3]);      
//		  	skb->data[test_post2] = braddr&0xff ;
//		  	skb->data[test_post2+1] = (braddr>>8)&0xff;
//		  	skb->data[test_post2+2] = (braddr>>16)&0xff ;
//		  	skb->data[test_post2+3] = (braddr>>24)&0xff ;
//				//printk("the new content =%x,%x,%x,%x ",skb->data[test_post2],skb->data[test_post2+1],skb->data[test_post2+2],skb->data[test_post2+3]);      
//		  	//printk("testpost2=%d:%x\n",test_post2,skb->data[test_post2]);
//		  	//test_post3 = test_post2+4;
//		  	//printk("testpost3=%d,uh->len=%d\n",test_post3,ntohs(uh->len));		  	
//		  			  	//printk("*********** Do dns hijack - src\n");
//		  }		  
//	  }
//	}
//}
//void dns_hijack_dst(struct sk_buff *skb)
//{
//	//struct sk_buff *skb = *pskb;	
//	struct iphdr *ip = skb->nh.iph;
//	//add by Allen
//	__u32 saddr=0, daddr=0;
//	__u16 src_port=0, dst_port=0;
//	int i;	
//	int change =0 ;
//	unsigned int udphoff =0 ;
//	int pos =0;
//	int dns_flags=0;	
//	int test_post1 =0, test_post2 = 0;
//	struct net_device *dev;
//	struct in_device *in_dev;
//	__u32 braddr;
//	struct in_ifaddr **ifap = NULL;
//	struct in_ifaddr *ifa = NULL;
//		
//	//add by Allen
//	if(ip->protocol == IPPROTO_UDP)
//	{
//		struct udphdr *uh=(struct udphdr *)((__u32 *)ip+ip->ihl);
//		src_port = ntohs(uh->source);
//		dst_port = ntohs(uh->dest);
//		udphoff = ip->ihl*4;	
//		
//		if(dst_port == 0x35)
//		{
//			//printk("UDP %d -> src %u.%u.%u.%u:%u,dest %u.%u.%u.%u:%u\n",ip->protocol,NIPQUAD(ip->saddr),src_port,NIPQUAD(ip->daddr),dst_port);
//			//printk("************* blankstatus = %d\n",blankstatus);
//			//check blankstatus
//			if( blankstatus == 1) //in blank status
//			{
//				change =2 ;
//				//parse string
//				for(i=0;i<(skb->len);i++)
//		    {		   
//		      //printk("%x ",skb->data[i]);
//			    if(skb->data[i] == xboxdomain_like[0] && skb->data[i+1] == xboxdomain_like[1] && skb->data[i+2] == xboxdomain_like[2]
//		      && skb->data[i+3] == xboxdomain_like[3] && skb->data[i+4] == xboxdomain_like[4] && skb->data[i+5] == xboxdomain_like[5]
//		      && skb->data[i+6] == xboxdomain_like[6] && skb->data[i+7] == xboxdomain_like[7] && skb->data[i+8] == xboxdomain_like[8]
//		      && skb->data[i+9] == xboxdomain_like[9] && skb->data[i+10] == xboxdomain_like[10] && skb->data[i+11] == xboxdomain_like[11]
//		      && skb->data[i+12] == xboxdomain_like[12] 
//		      )
//		      {
//		      	if(skb->data[i-3] == xboxdomain_diff[0] && skb->data[i-2] == xboxdomain_diff[1]&& skb->data[i-1] == xboxdomain_diff[2]) 
//		   	    {
//		   	    	 //printk("\nGot the as.xboxlive.com\n");
//		   	     change =1;
//		   	     break;
//		   	    }
//		   	    if(skb->data[i-4] == xboxdomain_diff[3] && skb->data[i-3] == xboxdomain_diff[4] && skb->data[i-2] == xboxdomain_diff[5]&& skb->data[i-1] == xboxdomain_diff[6]) 
//		   	    {
//		   	    	 //printk("\nGot the tgs.xboxlive.com\n");
//		   	     change =1;
//		   	     break;
//		   	    }
//		   	    if(skb->data[i-5] == xboxdomain_diff[7] && skb->data[i-4] == xboxdomain_diff[8] && skb->data[i-3] == xboxdomain_diff[9] && skb->data[i-2] == xboxdomain_diff[10]&& skb->data[i-1] == xboxdomain_diff[11]) 
//		   	    {
//		   	    	 //printk("\nGot the macs.xboxlive.com\n");
//		   	     change =1;
//		   	     break;
//		   	    }
//		   	     
//		      }		      
//		    }
//			}
////			else if(blankstatus == 0)  // in none blank status
////			{
//				change = 1;
//		    //parse string
//		    for(i=0;i<(skb->len);i++)
//		    {		   
//		      //printk("%x ",skb->data[i]);
//			    if(skb->data[i] == dns2content[0] && skb->data[i+1] == dns2content[1] && skb->data[i+2] == dns2content[2]
//		      && skb->data[i+3] == dns2content[3] && skb->data[i+4] == dns2content[4] && skb->data[i+5] == dns2content[5]
//		      && skb->data[i+6] == dns2content[6] && skb->data[i+7] == dns2content[7] && skb->data[i+8] == dns2content[8]
//		      && skb->data[i+9] == dns2content[9] && skb->data[i+10] == dns2content[10] && skb->data[i+11] == dns2content[11]
//		      && skb->data[i+12] == dns2content[12] && skb->data[i+13] == dns2content[13] && skb->data[i+14] == dns2content[14]
//		      && skb->data[i+15] == dns2content[15] && skb->data[i+16] == dns2content[16] && skb->data[i+17] == dns2content[17]
//		      )
//		      {
//		   	     //printk("\nI got the www.mywifiext.com\n");
//		   	     change =2;
//		   	     break;
//		      }
//		      else if(skb->data[i] == dns2content[4] && skb->data[i+1] == dns2content[5]
//		      && skb->data[i+2] == dns2content[6] && skb->data[i+3] == dns2content[7] && skb->data[i+4] == dns2content[8]
//		      && skb->data[i+5] == dns2content[9] && skb->data[i+6] == dns2content[10] && skb->data[i+7] == dns2content[11]
//		      && skb->data[i+8] == dns2content[12] && skb->data[i+9] == dns2content[13] && skb->data[i+10] == dns2content[14]
//		      && skb->data[i+11] == dns2content[15] && skb->data[i+12] == dns2content[16] && skb->data[i+13] == dns2content[17]
//		      )		     
//		      {
//		        //printk("\nI got the mywifiext.com\n");
//		        change =2 ;
//		        break;
//		      }		   
//          if(skb->data[i] == dns1content[0] && skb->data[i+1] == dns1content[1] && skb->data[i+2] == dns1content[2]
//		      && skb->data[i+3] == dns1content[3] && skb->data[i+4] == dns1content[4] && skb->data[i+5] == dns1content[5]
//		      && skb->data[i+6] == dns1content[6] && skb->data[i+7] == dns1content[7] && skb->data[i+8] == dns1content[8]
//		      && skb->data[i+9] == dns1content[9] && skb->data[i+10] == dns1content[10] && skb->data[i+11] == dns1content[11]
//		      && skb->data[i+12] == dns1content[12] && skb->data[i+13] == dns1content[13] && skb->data[i+14] == dns1content[14]
//		      && skb->data[i+15] == dns1content[15] && skb->data[i+16] == dns1content[16] && skb->data[i+17] == dns1content[17]
//		      )			 
//		      {
//		   	     //printk("\nI got the www.mywifiext.net\n");
//		   	     change =2;
//		   	     break;
//		      }
//		      if(skb->data[i] == dns1content[4] && skb->data[i+1] == dns1content[5]
//		      && skb->data[i+2] == dns1content[6] && skb->data[i+3] == dns1content[7] && skb->data[i+4] == dns1content[8]
//		      && skb->data[i+5] == dns1content[9] && skb->data[i+6] == dns1content[10] && skb->data[i+7] == dns1content[11]
//		      && skb->data[i+8] == dns1content[12] && skb->data[i+9] == dns1content[13] && skb->data[i+10] == dns1content[14]
//		      && skb->data[i+11] == dns1content[15] && skb->data[i+12] == dns1content[16] && skb->data[i+13] == dns1content[17]
//		      )
//		      {
//		        //printk("\nI got the mywifiext.net\n");
//		        change =2 ;
//		        break;
//		      }		      
//		    }
////		  }
//		  //check change 
//		  if (change == 2 )
//		  {
//		  	dev = dev_get_by_name("br0");
//		  	in_dev = in_dev_get(dev);
//        for (ifap = &in_dev->ifa_list; (ifa = *ifap) != NULL;ifap = &ifa->ifa_next) 
//        {
//         	if(!strcmp(ifa->ifa_label,"br0"))
//          	 braddr = ifa->ifa_local;
//        }				
//		  	//printk("************* br0'ip : %u.%u.%u.%u\n",NIPQUAD(braddr));			
//			//chage ip
//			ip->daddr=braddr;			
//		  	//if( blankstatus == 1)
//		  	 //printk("Need to do dns hijack\n");		  	 				
//		  	//ip->tot_len = 0xa000; // 160
//		  	ip->check = 0x0000;
//		  	//count the ip's checksum again
//		  	ip->check =(ip_fast_csum((u8 *)ip, ip->ihl));
//		  	//printk("ip header checksum is %lx\n",ip->check);
//			//printk("after UDP %d -> src %u.%u.%u.%u:%u,dest %u.%u.%u.%u:%u\n",ip->protocol,NIPQUAD(ip->saddr),src_port,NIPQUAD(ip->daddr),dst_port);
//		  	//write data to packet		    
//		  	uh->check=0;
//		      skb->ip_summed = CHECKSUM_UNNECESSARY;
//		  	//printk("\nModify and Add DNS response\n");
//                  //printk("*********** Do dns hijack - dst\n"); 	  	
//		  }		  
//	  }
//	}
//}
//#endif // DNS_HIJACK  
/*
 * Called via br_handle_frame_hook.
 * Return 0 if *pskb should be processed furthur
 *	  1 if *pskb is handled
 * note: already called with rcu_read_lock (preempt_disabled)
 */
int br_handle_frame(struct net_bridge_port *p, struct sk_buff **pskb)
{
	struct sk_buff *skb = *pskb;
	const unsigned char *dest = eth_hdr(skb)->h_dest;

//        //test by Allen
//        #ifdef DNS_HIJACK
//        dns_hijack_dst(skb);
//        dns_hijack_src(skb);
//        #endif // DNS_HIJACK

	if (!is_valid_ether_addr(eth_hdr(skb)->h_source))
		goto err;

	if (unlikely(is_link_local(dest))) {
		skb->pkt_type = PACKET_HOST;
		return NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_IN, skb, skb->dev,
			       NULL, br_handle_local_finish) != 0;
	}

	if (p->state == BR_STATE_FORWARDING || p->state == BR_STATE_LEARNING) {
		if (br_should_route_hook) {
			if (br_should_route_hook(pskb))
				return 0;
			skb = *pskb;
			dest = eth_hdr(skb)->h_dest;
		}

		if (!compare_ether_addr(p->br->dev->dev_addr, dest))
			skb->pkt_type = PACKET_HOST;

		NF_HOOK(PF_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_frame_finish);
		return 1;
	}

err:
	kfree_skb(skb);
	return 1;
}
