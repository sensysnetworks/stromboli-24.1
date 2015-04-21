/*
    rtnet/module/include/linux/rtnet.h
    rtnet header file

    rtnet - real-time networking subsystem
    Copyright (C) 1999,2000 Zentropic Computing, LLC

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __RTNET_H_
#define __RTNET_H_

#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <net/ip.h>

#include <rtai.h>
//#include <rtai_fifos.h>

#endif

/* some configurables */
#define RTNET_TASK_PRIORITY	0

#define RTNET_NUM_SOCKETS	10


#ifdef __KERNEL__

//struct rtskb;
struct rtskb_head;
struct rtnet_device;


#define rtskb sk_buff
#if 0
struct rtskb {
	struct rtskb	*next;
	struct rtskb	*prev;

	struct rtskb_head	*head;

	struct rt_sock	*sk;

	//struct timeval stamp;
	
	struct rtnet_device	*dev;

	/* transport layer */
	union
	{
		struct tcphdr	*th;
		struct udphdr	*uh;
		struct icmphdr	*icmph;
		struct iphdr	*ipihdr;
		unsigned char	*raw;
	} h;

	/* network layer */
	union
	{
		struct iphdr	*iph;
		struct arphdr	*arph;
		unsigned char	*raw;
	} nh;

	/* link layer */
	union
	{
		struct ethhdr	*ethernet;
		unsigned char	*raw;
	} mac;

	unsigned int	users;

	unsigned int	buf_len;
	unsigned char	*buf_ptr;

	unsigned int	data_len;
	unsigned char	*data_ptr;
};
#endif

struct rtskb_head {
	struct rtskb	*next;
	struct rtskb	*prev;

	unsigned int	qlen;
	spinlock_t	lock;
};

struct rtsocket {
	struct rtsocket *next;

	int use_count;
	int fd;

	unsigned short proto;

	struct rtsocket_ops	*ops;
	int (*deliver)(struct rtsocket *s,struct rtskb *skb);

	struct rtskb_head	incoming;

	u16 num;	/* socket number */

	u32 saddr;
	u32 rcv_saddr;
	u32 daddr;
	u16 sport;
	u16 dport;

	int ip_tos;
	int ip_ttl;

	int dead;
	int state;

	int (*wakeup)(int s,void *arg);
	void *private;
};

struct rtsocket_ops {
	int (*bind)(struct rtsocket *s,struct sockaddr *my_addr,int addrlen);
	int (*connect)(struct rtsocket *s,struct sockaddr *serv_addr,int addrlen);
	int (*listen)(struct rtsocket *s,int backlog);
	int (*recvmsg)(struct rtsocket *s,struct msghdr *msg,int len);
	int (*sendmsg)(struct rtsocket *s,const struct msghdr *msg,int len);

	void (*close)(struct rtsocket *s,long timeout);
};

/*
 *  we have two routing tables, the generic routing table and the
 *  specific routing table.  The specific routing table is the table
 *  actually used to route outgoing packets.  The generic routing
 *  table is used to discover specific routes when needed.
 */
struct rt_rtable{
	struct rt_rtable *	next;
	unsigned int 		use_count;

	__u32			rt_dst;
	__u32			rt_dst_mask;
	__u32			rt_src;
	int			rt_iif;
	int			rt_oif;

	struct rtnet_device *	rt_dev;
	char			rt_dst_mac_addr[6];
};


struct rtnet_device{
	struct rtnet_device *	next;
	__u32			local_addr;	/* in network order */
	struct net_device *	dev;

	struct rtsocket *	rx_sock;
	struct rtsocket *	protocols;

	struct rtsocket *	tx_sock;

	int			(*open)(struct rtnet_device *rtdev);
	void			(*stop)(struct rtnet_device *rtdev);
	int			(*hard_header)(struct rtskb *,struct rtnet_device *,unsigned 
					short type,void *daddr,void *saddr,unsigned int len);
	int			(*xmit)(struct rtskb *skb,struct rtnet_device *dev);
};


int rt_sock_register(struct net_proto_family *ops);


int rtnet_request_irq(unsigned int irq,void (*handler)(int,void *,struct pt_regs *),
		unsigned long flags,const char *device,void *dev_id);
void rtnet_free_irq(unsigned int irq,void *dev_id);

void kfree_rtskb(struct rtskb *);
struct rtskb *alloc_rtskb(unsigned int size,int gfp_mask);
struct rtskb *dev_alloc_rtskb(unsigned int size);
struct rtskb *rtskb_clone(struct rtskb *skb,int gfp_mask);
struct rtskb *rtskb_copy(struct rtskb *skb,int gfp_mask);
#define dev_kfree_rtskb(a)	kfree_rtskb(a)
void rtskb_queue_head_init(struct rtskb_head *list);
struct rtskb *rtskb_dequeue(struct rtskb_head *list);
int rtskb_queue_len(struct rtskb_head *list);
int rtskb_queue_empty(struct rtskb_head *list);
void rtskb_queue_tail(struct rtskb_head *list,struct rtskb *buf);

extern __inline__ struct rtskb *dev_alloc_rtskb(unsigned int size)
{
	return alloc_rtskb(size,GFP_KERNEL);
}


/* the external interface */

int rt_accept(int s,struct sockaddr *addr,int *addrlen);
int rt_bind(int s,struct sockaddr *my_addr,int addrlen);
int rt_close(int s);
int rt_connect(int s,struct sockaddr *serv_addr,int addrlen);
int rt_listen(int s,int backlog);
int rt_recv(int s,void *buf,int len,unsigned int flags);
int rt_recvfrom(int s,void *buf,int len,unsigned int flags,
		struct sockaddr *from,int *fromlen);
int rt_recvmsg(int s,struct msghdr *msg,unsigned int flags);
//int rt_send(int s,const void *msg,int len,unsigned int flags);
int rt_sendmsg(int s,const struct msghdr *msg,unsigned int flags);
int rt_sendto(int s,const void *buf,int len,unsigned int flags,
		struct sockaddr *to,int tolen);
int rt_shutdown(int s,int how);
int rt_socket(int domain,int type,int protocol);

int rt_socket_callback(int s,int (*func)(int s,void *arg),void *arg);

#if 0
int rt_getsockname(int s,struct sockaddr *name,int *namelen);
int rt_getsockopt(int s,int level,int optname,void *optval,int *optlen);
int rt_setsockopt(int s,int level,int optname,const void *optval,int optlen);

struct protoent *rt_getprotoent(void);
struct protoent *rt_getprotobyname(const char *name);
struct protoent *rt_getprotobynumber(int proto);
void rt_setprotoent(int stayopen);
void rt_endprotoent(void);
#endif


/* some defs */

#define MAX_SOCK_ADDR 128

#define N_RT_SOCKETS 16



#if 0
#define accept		rt_accept
#define bind		rt_bind
#define connect		rt_connect
#define listen		rt_listen
#define recv		rt_recv
#define recvfrom	rt_recvfrom
#define recvmsg		rt_recvmsg
#define send		rt_send
#define sendmsg		rt_sendmsg
#define sendto		rt_sendto
#define socket		rt_socket
#endif


void rt_ip_options_build(struct rtskb *skb,struct ip_options * opt,
		u32 daddr,struct rt_rtable *rt,int is_frag);

int rt_ip_build_xmit(struct rtsocket *sk,int getfrag(const void *,char *,
	unsigned int,unsigned int),const void *frag,unsigned length,
	struct ipcm_cookie *ipc,struct rt_rtable *rt,int flags);


int rt_dev_queue_xmit(struct rtskb *skb);


/* ip rx */

int rt_ip_rcv(struct rtsocket *dev,struct rtskb *);

/* arp stuff */

int rt_arp_solicit(struct rtnet_device *dev,u32 target);
void rt_arp_table_add(u32 ip_addr,void *hw_addr,int hw_addr_len);
void rt_arp_table_del(u32 ip_addr);

void rt_arp_table_init(void);

/* routing stuff */

struct rt_rtable *rt_ip_route_add(struct rtnet_device *iface,u32 daddr,u32 dmask);
struct rt_rtable *rt_ip_route_find(u32 daddr);
void rt_ip_route_del(struct rt_rtable *rt);

int rt_ip_route_input(struct rtskb *skb,u32 daddr,u32 saddr,u8 tos,struct rtnet_device *dev);
int rt_ip_route_output(struct rt_rtable **r,u32 daddr,u32 saddr,u32 tos,int oif);

/* dev stuff */

extern struct rtnet_device *rtnet_devices;
extern struct rtsocket *rt_protocols;

void rt_dev_up(const char *if_name,u32 local_addr);
void rt_dev_down(const char *if_name);
int rt_dev_open(struct rtnet_device *rtdev);
int rt_dev_close(struct rtnet_device *rtdev);
struct rtnet_device *rt_dev_alloc(struct net_device *dev);
struct rtnet_device *rt_dev_get(const char *if_name);
void rt_dev_add_pack(struct rtnet_device *rtdev,struct rtsocket *);
struct rtnet_device *rt_dev_get_by_dev(struct net_device *dev);
struct rtnet_device *rt_dev_get_by_index(int ifindex);
struct rtnet_device *rt_dev_getbyhwaddr(unsigned short type,char *ha);
int rt_dev_queue_xmit(struct rtskb *skb);
void rtnetif_rx(struct rtskb *skb);
void rtnetif_wake_queue(struct net_device *dev);
void rtnet_bh(void);
void rtnet_mark_rxtask(void);




/* eth stuff */

int rt_eth_header(struct rtskb *skb,struct rtnet_device *rtdev,
	unsigned short type,void *daddr,void *saddr,unsigned int len);

#endif

/* user interface for /dev/rtnet */

#define RTNET_MINOR	240

#define IOC_RT_IFUP		100
#define IOC_RT_IFDOWN		101
#define IOC_RT_IF		102
#define IOC_RT_ROUTE_ADD	103
#define IOC_RT_ROUTE_SOLICIT	104
#define IOC_RT_ROUTE_DELETE	105
#define IOC_RT_ROUTE_GET	106

struct rtnet_config{
	char if_name[16];
	int len;

	u32 ip_addr;
	u32 ip_mask;
	u32 ip_netaddr;
	u32 ip_broadcast;
};






#endif

