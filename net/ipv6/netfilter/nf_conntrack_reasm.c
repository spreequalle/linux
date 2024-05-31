/*
 * IPv6 fragment reassembly for connection tracking
 *
 * Copyright (C)2004 USAGI/WIDE Project
 *
 * Author:
 *	Yasuyuki Kozakai @USAGI <yasuyuki.kozakai@toshiba.co.jp>
 *
 * Based on: net/ipv6/reassembly.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/jiffies.h>
#include <linux/net.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/icmpv6.h>
#include <linux/random.h>
#include <linux/ipv6_route.h>
#include <linux/jhash.h>

#include <net/sock.h>
#include <net/snmp.h>
#include <net/inet_frag.h>
#include <net/ip6_route.h>

#include <net/ipv6.h>
#include <net/protocol.h>
#include <net/transp_v6.h>
#include <net/rawv6.h>
#include <net/ndisc.h>
#include <net/addrconf.h>
#include <linux/sysctl.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv6.h>
#include <linux/kernel.h>
#include <linux/module.h>

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

#define NF_CT_FRAG6_HIGH_THRESH 262144 /* == 256*1024 */
#define NF_CT_FRAG6_LOW_THRESH 196608  /* == 192*1024 */
#define NF_CT_FRAG6_TIMEOUT IPV6_FRAG_TIMEOUT

struct nf_ct_frag6_skb_cb
{
	struct inet6_skb_parm	h;
	int			offset;
	struct sk_buff		*orig;
};

#define NFCT_FRAG6_CB(skb)	((struct nf_ct_frag6_skb_cb*)((skb)->cb))

struct nf_ct_frag6_queue
{
	struct inet_frag_queue  q;

	__be32			id;		/* fragment id		*/
	u32					user;
	struct in6_addr		saddr;
	struct in6_addr		daddr;

	unsigned int		csum;
	__u16			nhoffset;
};

struct inet_frags_ctl nf_frags_ctl __read_mostly = {
       .high_thresh     = 256 * 1024,
       .low_thresh      = 192 * 1024,
       .timeout         = IPV6_FRAG_TIMEOUT,
       .secret_interval = 10 * 60 * HZ,
};


/* Hash table. */

static struct inet_frags nf_frags;

static unsigned int ip6qhashfn(__be32 id, struct in6_addr *saddr,
			       struct in6_addr *daddr)
{
	u32 a, b, c;

	a = (__force u32)saddr->s6_addr32[0];
	b = (__force u32)saddr->s6_addr32[1];
	c = (__force u32)saddr->s6_addr32[2];

	a += JHASH_GOLDEN_RATIO;
	b += JHASH_GOLDEN_RATIO;
	c += nf_frags.rnd;
	__jhash_mix(a, b, c);

	a += (__force u32)saddr->s6_addr32[3];
	b += (__force u32)daddr->s6_addr32[0];
	c += (__force u32)daddr->s6_addr32[1];
	__jhash_mix(a, b, c);

	a += (__force u32)daddr->s6_addr32[2];
	b += (__force u32)daddr->s6_addr32[3];
	c += (__force u32)id;
	__jhash_mix(a, b, c);

	return c & (INETFRAGS_HASHSZ - 1);
}

static unsigned int nf_hashfn(struct inet_frag_queue *q)
{
	struct nf_ct_frag6_queue *nq;

	nq = container_of(q, struct nf_ct_frag6_queue, q);
	return ip6qhashfn(nq->id, &nq->saddr, &nq->daddr);
}

static void nf_skb_free(struct sk_buff *skb)
{
       if (NFCT_FRAG6_CB(skb)->orig)
               kfree_skb(NFCT_FRAG6_CB(skb)->orig);
}

/* Memory Tracking Functions. */
static inline void frag_kfree_skb(struct sk_buff *skb, unsigned int *work)
{
	if (work)
		*work -= skb->truesize;
	atomic_sub(skb->truesize, &nf_frags.mem);

	nf_skb_free(skb);
	kfree_skb(skb);
}

static void nf_frag_free(struct inet_frag_queue *q)
{
	kfree(container_of(q, struct nf_ct_frag6_queue, q));
}

/* Destruction primitives. */

/* Complete destruction of fq. */

static __inline__ void fq_put(struct nf_ct_frag6_queue *fq, unsigned int *work)
{
	if (atomic_dec_and_test(&fq->q.refcnt))
		inet_frag_destroy(&fq->q, &nf_frags, work);
}

/* Kill fq entry. It is not destroyed immediately,
 * because caller (and someone more) holds reference count.
 */
static __inline__ void fq_kill(struct nf_ct_frag6_queue *fq)
{
	inet_frag_kill(&fq->q, &nf_frags);
}

static void nf_ct_frag6_evictor(void)
{
	struct nf_ct_frag6_queue *fq;
	struct list_head *tmp;
	unsigned int work;

	work = atomic_read(&nf_frags.mem);
	if (work <= nf_frags_ctl.low_thresh)
		return;

	work -= nf_frags_ctl.low_thresh;
	while (work > 0) {
		read_lock(&nf_frags.lock);
		if (list_empty(&nf_frags.lru_list)) {
			read_unlock(&nf_frags.lock);
			return;
		}
		tmp = nf_frags.lru_list.next;
		BUG_ON(tmp == NULL);
		fq = list_entry(tmp, struct nf_ct_frag6_queue, q.lru_list);
		atomic_inc(&fq->q.refcnt);
		read_unlock(&nf_frags.lock);

		spin_lock(&fq->q.lock);
		if (!(fq->q.last_in&COMPLETE))
			fq_kill(fq);
		spin_unlock(&fq->q.lock);

		fq_put(fq, &work);
	}
}

static void nf_ct_frag6_expire(unsigned long data)
{
	struct nf_ct_frag6_queue *fq;
	
	fq = container_of((struct inet_frag_queue *)data,
				struct nf_ct_frag6_queue, q);

	spin_lock(&fq->q.lock);

	if (fq->q.last_in & COMPLETE)
		goto out;

	fq_kill(fq);

	/* Don't send error if the first segment did not arrive. */
	if (!(fq->q.last_in & FIRST_IN) || !fq->q.fragments)
		goto out;

	/*
	 * Only search router table for the head fragment,
	 * when defraging timeout at PRE_ROUTING HOOK.
	 */
	if (fq->user == IP6_DEFRAG_CONNTRACK_IN) {
		struct sk_buff *head = fq->q.fragments;

		ip6_route_input(head);
		if (!skb_dst(head))
			goto out;

		/*
		 * Only an end host needs to send an ICMP "Fragment Reassembly
		 * Timeout" message, per section 4.5 of RFC2460.
		 */
		if (!(skb_r6table(head)->rt6i_flags & RTF_LOCAL))
			goto out;

		/* Send an ICMP "Fragment Reassembly Timeout" message. */
		icmpv6_send(head, ICMPV6_TIME_EXCEED, ICMPV6_EXC_FRAGTIME, 0,
			    head->dev);
	}


out:
	spin_unlock(&fq->q.lock);
	fq_put(fq, NULL);
}

/* Creation primitives. */

static __inline__ struct nf_ct_frag6_queue *
		fq_find(__be32 id, u32 user, struct in6_addr *src, struct in6_addr *dst)
{
	struct inet_frag_queue *q;
	struct ip6_create_arg arg;
	unsigned int hash;

	arg.id = id;
	arg.user = user;
	arg.src = src;
	arg.dst = dst;
	hash = ip6qhashfn(id, src, dst);

	q = inet_frag_find(&nf_frags, &arg, hash);
	if (q == NULL)
		goto oom;

	return container_of(q, struct nf_ct_frag6_queue, q);

oom:
	pr_debug("Can't alloc new queue\n");
	return NULL;
}

static int nf_ct_frag6_queue(struct nf_ct_frag6_queue *fq, struct sk_buff *skb,
			     struct frag_hdr *fhdr, int nhoff)
{
	struct sk_buff *prev, *next;
	int offset, end;

	if (fq->q.last_in & COMPLETE) {
		DEBUGP("Allready completed\n");
		goto err;
	}

	offset = ntohs(fhdr->frag_off) & ~0x7;
	end = offset + (ntohs(skb->nh.ipv6h->payload_len) -
			((u8 *) (fhdr + 1) - (u8 *) (skb->nh.ipv6h + 1)));

	if ((unsigned int)end > IPV6_MAXPLEN) {
		DEBUGP("offset is too large.\n");
		return -1;
	}

	if (skb->ip_summed == CHECKSUM_COMPLETE)
		skb->csum = csum_sub(skb->csum,
				     csum_partial(skb->nh.raw,
						  (u8*)(fhdr + 1) - skb->nh.raw,
						  0));

	/* Is this the final fragment? */
	if (!(fhdr->frag_off & htons(IP6_MF))) {
		/* If we already have some bits beyond end
		 * or have different end, the segment is corrupted.
		 */
		if (end < fq->q.len ||
		    ((fq->q.last_in & LAST_IN) && end != fq->q.len)) {
			DEBUGP("already received last fragment\n");
			goto err;
		}
		fq->q.last_in |= LAST_IN;
		fq->q.len = end;
	} else {
		/* Check if the fragment is rounded to 8 bytes.
		 * Required by the RFC.
		 */
		if (end & 0x7) {
			/* RFC2460 says always send parameter problem in
			 * this case. -DaveM
			 */
			DEBUGP("the end of this fragment is not rounded to 8 bytes.\n");
			return -1;
		}
		if (end > fq->q.len) {
			/* Some bits beyond end -> corruption. */
			if (fq->q.last_in & LAST_IN) {
				DEBUGP("last packet already reached.\n");
				goto err;
			}
			fq->q.len = end;
		}
	}

	if (end == offset)
		goto err;

	/* Point into the IP datagram 'data' part. */
	if (!pskb_pull(skb, (u8 *) (fhdr + 1) - skb->data)) {
		DEBUGP("queue: message is too short.\n");
		goto err;
	}
	if (pskb_trim_rcsum(skb, end - offset)) {
		DEBUGP("Can't trim\n");
		goto err;
	}

	/* Find out which fragments are in front and at the back of us
	 * in the chain of fragments so far.  We must know where to put
	 * this fragment, right?
	 */
	prev = NULL;
	for (next = fq->q.fragments; next != NULL; next = next->next) {
		if (NFCT_FRAG6_CB(next)->offset >= offset)
			break;	/* bingo! */
		prev = next;
	}

	/* We found where to put this one.  Check for overlap with
	 * preceding fragment, and, if needed, align things so that
	 * any overlaps are eliminated.
	 */
	if (prev) {
		int i = (NFCT_FRAG6_CB(prev)->offset + prev->len) - offset;

		if (i > 0) {
			offset += i;
			if (end <= offset) {
				DEBUGP("overlap\n");
				goto err;
			}
			if (!pskb_pull(skb, i)) {
				DEBUGP("Can't pull\n");
				goto err;
			}
			if (skb->ip_summed != CHECKSUM_UNNECESSARY)
				skb->ip_summed = CHECKSUM_NONE;
		}
	}

	/* Look for overlap with succeeding segments.
	 * If we can merge fragments, do it.
	 */
	while (next && NFCT_FRAG6_CB(next)->offset < end) {
		/* overlap is 'i' bytes */
		int i = end - NFCT_FRAG6_CB(next)->offset;

		if (i < next->len) {
			/* Eat head of the next overlapped fragment
			 * and leave the loop. The next ones cannot overlap.
			 */
			DEBUGP("Eat head of the overlapped parts.: %d", i);
			if (!pskb_pull(next, i))
				goto err;

			/* next fragment */
			NFCT_FRAG6_CB(next)->offset += i;
			fq->q.meat -= i;
			if (next->ip_summed != CHECKSUM_UNNECESSARY)
				next->ip_summed = CHECKSUM_NONE;
			break;
		} else {
			struct sk_buff *free_it = next;

			/* Old fragmnet is completely overridden with
			 * new one drop it.
			 */
			next = next->next;

			if (prev)
				prev->next = next;
			else
				fq->q.fragments = next;

			fq->q.meat -= free_it->len;
			frag_kfree_skb(free_it, NULL);
		}
	}

	NFCT_FRAG6_CB(skb)->offset = offset;

	/* Insert this fragment in the chain of fragments. */
	skb->next = next;
	if (prev)
		prev->next = skb;
	else
		fq->q.fragments = skb;

	skb_get_timestamp(skb, &fq->q.stamp);
	fq->q.meat += skb->len;
	atomic_add(skb->truesize, &nf_frags.mem);

	/* The first fragment.
	 * nhoffset is obtained from the first fragment, of course.
	 * Reserve dev for sending an ICMP "Fragment Reassembly Timeout"
	 * message.
	 */
	if (offset == 0) {
		fq->nhoffset = nhoff;
		fq->q.last_in |= FIRST_IN;
	} else {
		skb->dev = NULL;
	}

	write_lock(&nf_frags.lock);
	list_move_tail(&fq->q.lru_list, &nf_frags.lru_list);
	write_unlock(&nf_frags.lock);
	return 0;

err:
	return -1;
}

/*
 *	Check if this packet is complete.
 *	Returns NULL on failure by any reason, and pointer
 *	to current nexthdr field in reassembled frame.
 *
 *	It is called with locked fq, and caller must check that
 *	queue is eligible for reassembly i.e. it is not COMPLETE,
 *	the last and the first frames arrived and all the bits are here.
 */
static struct sk_buff *
nf_ct_frag6_reasm(struct nf_ct_frag6_queue *fq, struct net_device *dev)
{
	struct sk_buff *fp, *op, *head = fq->q.fragments;
	int    payload_len;

	fq_kill(fq);

	BUG_TRAP(head != NULL);
	BUG_TRAP(NFCT_FRAG6_CB(head)->offset == 0);

	/* Unfragmented part is taken from the first segment. */
	payload_len = (head->data - head->nh.raw) - sizeof(struct ipv6hdr) + fq->q.len - sizeof(struct frag_hdr);
	if (payload_len > IPV6_MAXPLEN) {
		DEBUGP("payload len is too large.\n");
		goto out_oversize;
	}

	/* Head of list must not be cloned. */
	if (skb_cloned(head) && pskb_expand_head(head, 0, 0, GFP_ATOMIC)) {
		DEBUGP("skb is cloned but can't expand head");
		goto out_oom;
	}

	/* If the first fragment is fragmented itself, we split
	 * it to two chunks: the first with data and paged part
	 * and the second, holding only fragments. */
	if (skb_shinfo(head)->frag_list) {
		struct sk_buff *clone;
		int i, plen = 0;

		if ((clone = alloc_skb(0, GFP_ATOMIC)) == NULL) {
			DEBUGP("Can't alloc skb\n");
			goto out_oom;
		}
		clone->next = head->next;
		head->next = clone;
		skb_shinfo(clone)->frag_list = skb_shinfo(head)->frag_list;
		skb_shinfo(head)->frag_list = NULL;
		for (i=0; i<skb_shinfo(head)->nr_frags; i++)
			plen += skb_shinfo(head)->frags[i].size;
		clone->len = clone->data_len = head->data_len - plen;
		head->data_len -= clone->len;
		head->len -= clone->len;
		clone->csum = 0;
		clone->ip_summed = head->ip_summed;

		NFCT_FRAG6_CB(clone)->orig = NULL;
		atomic_add(clone->truesize, &nf_frags.mem);
	}

	/* We have to remove fragment header from datagram and to relocate
	 * header in order to calculate ICV correctly. */
	head->nh.raw[fq->nhoffset] = head->h.raw[0];
	memmove(head->head + sizeof(struct frag_hdr), head->head,
		(head->data - head->head) - sizeof(struct frag_hdr));
	head->mac.raw += sizeof(struct frag_hdr);
	head->nh.raw += sizeof(struct frag_hdr);

	skb_shinfo(head)->frag_list = head->next;
	head->h.raw = head->data;
	skb_push(head, head->data - head->nh.raw);
	atomic_sub(head->truesize, &nf_frags.mem);

	for (fp=head->next; fp; fp = fp->next) {
		head->data_len += fp->len;
		head->len += fp->len;
		if (head->ip_summed != fp->ip_summed)
			head->ip_summed = CHECKSUM_NONE;
		else if (head->ip_summed == CHECKSUM_COMPLETE)
			head->csum = csum_add(head->csum, fp->csum);
		head->truesize += fp->truesize;
		atomic_sub(fp->truesize, &nf_frags.mem);
	}

	head->next = NULL;
	head->dev = dev;
	skb_set_timestamp(head, &fq->q.stamp);
	head->nh.ipv6h->payload_len = htons(payload_len);

	/* Yes, and fold redundant checksum back. 8) */
	if (head->ip_summed == CHECKSUM_COMPLETE)
		head->csum = csum_partial(head->nh.raw, head->h.raw-head->nh.raw, head->csum);

	fq->q.fragments = NULL;

	/* all original skbs are linked into the NFCT_FRAG6_CB(head).orig */
	fp = skb_shinfo(head)->frag_list;
	if (NFCT_FRAG6_CB(fp)->orig == NULL)
		/* at above code, head skb is divided into two skbs. */
		fp = fp->next;

	op = NFCT_FRAG6_CB(head)->orig;
	for (; fp; fp = fp->next) {
		struct sk_buff *orig = NFCT_FRAG6_CB(fp)->orig;

		op->next = orig;
		op = orig;
		NFCT_FRAG6_CB(fp)->orig = NULL;
	}

	return head;

out_oversize:
	if (net_ratelimit())
		printk(KERN_DEBUG "nf_ct_frag6_reasm: payload len = %d\n", payload_len);
	goto out_fail;
out_oom:
	if (net_ratelimit())
		printk(KERN_DEBUG "nf_ct_frag6_reasm: no memory for reassembly\n");
out_fail:
	return NULL;
}

/*
 * find the header just before Fragment Header.
 *
 * if success return 0 and set ...
 * (*prevhdrp): the value of "Next Header Field" in the header
 *		just before Fragment Header.
 * (*prevhoff): the offset of "Next Header Field" in the header
 *		just before Fragment Header.
 * (*fhoff)   : the offset of Fragment Header.
 *
 * Based on ipv6_skip_hdr() in net/ipv6/exthdr.c
 *
 */
static int
find_prev_fhdr(struct sk_buff *skb, u8 *prevhdrp, int *prevhoff, int *fhoff)
{
	u8 nexthdr = skb->nh.ipv6h->nexthdr;
	u8 prev_nhoff = (u8 *)&skb->nh.ipv6h->nexthdr - skb->data;
	int start = (u8 *)(skb->nh.ipv6h+1) - skb->data;
	int len = skb->len - start;
	u8 prevhdr = NEXTHDR_IPV6;

	while (nexthdr != NEXTHDR_FRAGMENT) {
		struct ipv6_opt_hdr hdr;
		int hdrlen;

		if (!ipv6_ext_hdr(nexthdr)) {
			return -1;
		}
		if (len < (int)sizeof(struct ipv6_opt_hdr)) {
			DEBUGP("too short\n");
			return -1;
		}
		if (nexthdr == NEXTHDR_NONE) {
			DEBUGP("next header is none\n");
			return -1;
		}
		if (skb_copy_bits(skb, start, &hdr, sizeof(hdr)))
			BUG();
		if (nexthdr == NEXTHDR_AUTH)
			hdrlen = (hdr.hdrlen+2)<<2;
		else
			hdrlen = ipv6_optlen(&hdr);

		prevhdr = nexthdr;
		prev_nhoff = start;

		nexthdr = hdr.nexthdr;
		len -= hdrlen;
		start += hdrlen;
	}

	if (len < 0)
		return -1;

	*prevhdrp = prevhdr;
	*prevhoff = prev_nhoff;
	*fhoff = start;

	return 0;
}

struct sk_buff *nf_ct_frag6_gather(struct sk_buff *skb, u32 user)
{
	struct sk_buff *clone;
	struct net_device *dev = skb->dev;
	struct frag_hdr *fhdr;
	struct nf_ct_frag6_queue *fq;
	struct ipv6hdr *hdr;
	int fhoff, nhoff;
	u8 prevhdr;
	struct sk_buff *ret_skb = NULL;

	/* Jumbo payload inhibits frag. header */
	if (skb->nh.ipv6h->payload_len == 0) {
		DEBUGP("payload len = 0\n");
		return skb;
	}

	if (find_prev_fhdr(skb, &prevhdr, &nhoff, &fhoff) < 0)
		return skb;

	clone = skb_clone(skb, GFP_ATOMIC);
	if (clone == NULL) {
		DEBUGP("Can't clone skb\n");
		return skb;
	}

	NFCT_FRAG6_CB(clone)->orig = skb;

	if (!pskb_may_pull(clone, fhoff + sizeof(*fhdr))) {
		DEBUGP("message is too short.\n");
		goto ret_orig;
	}

	clone->h.raw = clone->data + fhoff;
	hdr = clone->nh.ipv6h;
	fhdr = (struct frag_hdr *)clone->h.raw;

	if (!(fhdr->frag_off & htons(0xFFF9))) {
		DEBUGP("Invalid fragment offset\n");
		/* It is not a fragmented frame */
		goto ret_orig;
	}

	if (atomic_read(&nf_frags.mem) > nf_frags_ctl.high_thresh)
		nf_ct_frag6_evictor();

	fq = fq_find(fhdr->identification, user, &hdr->saddr, &hdr->daddr);
	if (fq == NULL) {
		DEBUGP("Can't find and can't create new queue\n");
		goto ret_orig;
	}

	spin_lock(&fq->q.lock);

	if (nf_ct_frag6_queue(fq, clone, fhdr, nhoff) < 0) {
		spin_unlock(&fq->q.lock);
		DEBUGP("Can't insert skb to queue\n");
		fq_put(fq, NULL);
		goto ret_orig;
	}

	if (fq->q.last_in == (FIRST_IN|LAST_IN) && fq->q.meat == fq->q.len) {
		ret_skb = nf_ct_frag6_reasm(fq, dev);
		if (ret_skb == NULL)
			DEBUGP("Can't reassemble fragmented packets\n");
	}
	spin_unlock(&fq->q.lock);

	fq_put(fq, NULL);
	return ret_skb;

ret_orig:
	kfree_skb(clone);
	return skb;
}

void nf_ct_frag6_output(unsigned int hooknum, struct sk_buff *skb,
			struct net_device *in, struct net_device *out,
			int (*okfn)(struct sk_buff *))
{
	struct sk_buff *s, *s2;

	for (s = NFCT_FRAG6_CB(skb)->orig; s;) {
		nf_conntrack_put_reasm(s->nfct_reasm);
		nf_conntrack_get_reasm(skb);
		s->nfct_reasm = skb;

		s2 = s->next;
		s->next = NULL;

		NF_HOOK_THRESH(PF_INET6, hooknum, s, in, out, okfn,
			       NF_IP6_PRI_CONNTRACK_DEFRAG + 1);
		s = s2;
	}
	nf_conntrack_put_reasm(skb);
}

int nf_ct_frag6_kfree_frags(struct sk_buff *skb)
{
	struct sk_buff *s, *s2;

	for (s = NFCT_FRAG6_CB(skb)->orig; s; s = s2) {

		s2 = s->next;
		kfree_skb(s);
	}

	kfree_skb(skb);

	return 0;
}

int nf_ct_frag6_init(void)
{
	nf_frags.ctl = &nf_frags_ctl;
	nf_frags.hashfn = nf_hashfn;
	nf_frags.destructor = nf_frag_free;
	nf_frags.skb_free = nf_skb_free;
	nf_frags.qsize = sizeof(struct nf_ct_frag6_queue);
	nf_frags.constructor = ip6_frag_init;
	nf_frags.equal = ip6_frag_equal;
	nf_frags.match = ip6_frag_match;
	nf_frags.frag_expire = nf_ct_frag6_expire;
	
	inet_frags_init(&nf_frags);

	return 0;
}

void nf_ct_frag6_cleanup(void)
{
	inet_frags_fini(&nf_frags);
	
	nf_frags_ctl.low_thresh = 0;
	nf_ct_frag6_evictor();
}
