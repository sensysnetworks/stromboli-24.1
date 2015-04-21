/*
rtnet/dev.c - implement a networking device
Copyright (C) 1999      Lineo, Inc
              1999,2002 David A. Schleef <ds@schleef.org>

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <rtnet.h>
#include <rtnet_internal.h>

struct rtnet_device *rtnet_devices;
struct rtsocket *rt_protocols;

static struct rtskb_head backlog;

void rt_dev_add_pack(struct rtnet_device *rtdev,struct rtsocket *sock)
{
	sock->next=rt_protocols;
	rt_protocols=sock;
}

void rt_dev_remove_pack(struct packet_type *pt)
{
	printk("rtnet: bug: dev_remove_pack() not implemented\n");
}

struct rtnet_device *rt_dev_get(const char *name)
{
	return rt_dev_get_by_dev(dev_get_by_name(name));
}

struct rtnet_device * rt_dev_get_by_index(int ifindex)
{
	return rt_dev_get_by_dev(dev_get_by_index(ifindex));
}

struct rtnet_device * rt_dev_get_by_dev(struct net_device *dev)
{
	struct rtnet_device * rtdev;

	for(rtdev=rtnet_devices;rtdev;rtdev=rtdev->next){
		if(rtdev->dev==dev){
			return rtdev;
		}
	}
	return NULL;
}

struct rtnet_device *rt_dev_getbyhwaddr(unsigned short type, char *ha)
{
	struct rtnet_device * rtdev;
	struct net_device *dev;

	for(rtdev=rtnet_devices;rtdev;rtdev=rtdev->next){
		dev=rtdev->dev;
		if (dev->type == type &&
		    memcmp(dev->dev_addr, ha, dev->addr_len) == 0)
			return rtdev;
	}
	return NULL;
}


/*
 *	Create a new interface
 */
 
struct rtnet_device *rt_dev_alloc(struct net_device *dev)
{
	struct rtnet_device *rtdev;

	rtdev=kmalloc(sizeof(*rtdev),GFP_KERNEL);
	if(!rtdev)return NULL;
	memset(rtdev,0,sizeof(*rtdev));

	rtdev->dev=dev;

	rtdev->next=rtnet_devices;
	rtnet_devices=rtdev;

	rtdev->rx_sock=rtsocket_alloc();
	rtdev->tx_sock=rtsocket_alloc();

	return rtdev;
}


/*
 *	Prepare an interface for use. 
 */
 
int rt_dev_open(struct rtnet_device *rtdev)
{
	int ret = 0;
	struct net_device *dev=rtdev->dev;

	/*
	 *	Is it already up?
	 */

	if (dev->flags&IFF_UP)
		return 0;

	/*
	 *	Call device private open method
	 */
	 
	if (rtdev->open) 
  		ret = rtdev->open(rtdev);

	/*
	 *	If it went open OK then:
	 */
	 
	if (ret == 0) 
	{
		/*
		 *	nil rebuild_header routine,
		 *	that should be never called and used as just bug trap.
		 */

#if 0
		if (dev->rebuild_header == NULL)
			dev->rebuild_header = default_rebuild_header;
#endif

		/*
		 *	Set the flags.
		 */
		dev->flags |= (IFF_UP | IFF_RUNNING);

#if 0
		/*
		 *	Initialize multicasting status 
		 */
		dev_mc_upload(dev);
#endif

#if 0
		/*
		 *	Wakeup transmit queue engine
		 */
		dev_activate(dev);
#endif

#if 0
		/*
		 *	... and announce new interface.
		 */
		notifier_call_chain(&netdev_chain, NETDEV_UP, dev);
#endif

	}
	return(ret);
}

/*
 *	Completely shutdown an interface.
 */
 
int rt_dev_close(struct rtnet_device *rtdev)
{
	//struct net_device *dev=rtdev->dev;

	if (!(rtdev->dev->flags&IFF_UP))
		return 0;

	if (rtdev->stop)
		rtdev->stop(rtdev);

	rtdev->dev->flags&=~(IFF_UP|IFF_RUNNING);

	return(0);
}


int rt_dev_queue_xmit(struct rtskb *skb)
{
	struct rtnet_device *rtdev = (struct rtnet_device *)skb->dev;
	int ret;

	ret=rtdev->xmit(skb,rtdev);
if(ret)rt_printk("xmit returned %d not 0\n",ret);

	return 0;
}



void rtnetif_rx(struct sk_buff *skb)
{
	struct rtnet_device *rtdev;

	rtdev=rt_dev_get_by_dev(skb->dev);
	if(rtdev){
		skb->dev=(void *)rtdev;
	}

#if 0
	if(skb->stamp.tv_sec==0)
		get_fast_time(&skb->stamp);
#endif

	if(rtskb_queue_len(&backlog)>=20){
		rt_printk("dropping packet in rtnetif_rx()\n");
		kfree_rtskb(skb);
	}
	if(skb){
		rtskb_queue_tail(&backlog,skb);

#ifdef CONFIG_RTAI_RTNET_TASK
		rtnet_mark_rxtask();
#else
		rtnet_bh();
#endif
	}else{
		rt_printk("called rtnetif_rx() with skb=NULL\n");
	}
}


void rtnetif_wake_queue(struct net_device *dev)
{
#ifdef CONFIG_RTAI_RTNET_TASK
	//rtnet_mark_txtask();
#endif
}

void rtnet_bh(void)
{
	struct rtsocket *ptype;
	struct rtsocket *pt_prev;
	unsigned short type;

//rt_printk("backlog %d\n",skb_queue_len(&backlog));
	while (!rtskb_queue_empty(&backlog)) 
	{
		struct rtskb * skb;

		skb = rtskb_dequeue(&backlog);

		/* XXX until we figure out every place to modify.. */
		skb->h.raw = skb->nh.raw = skb->data;

		if (skb->mac.raw < skb->head || skb->mac.raw > skb->data) {
			rt_printk(KERN_CRIT "%s: wrong mac.raw ptr, proto=%04x\n", skb->dev->name, skb->protocol);
			kfree_rtskb(skb);
			continue;
		}

		type = skb->protocol;

		/* since we only have a few protocols, we won't bother to
		   use the hash */

		pt_prev = NULL;
		for (ptype = rt_protocols; ptype!=NULL; ptype=ptype->next)
		{
			if (ptype->proto==skb->protocol) {
/* XXX check device */
				if(pt_prev)
				{
					struct rtskb *skb2 = rtskb_clone(skb, GFP_ATOMIC);
					if(skb2)
						pt_prev->deliver(pt_prev,skb2);
				}
				pt_prev=ptype;
			}
		}

		if(pt_prev){
			pt_prev->deliver(pt_prev,skb);
		} else {
			rt_printk("unknown packet\n");
			kfree_rtskb(skb);
		}
  	}	/* End of queue loop */
  	
	return;
}



int rt_net_dev_init(void)
{
	rtskb_queue_head_init(&backlog);

	return 0;
}

