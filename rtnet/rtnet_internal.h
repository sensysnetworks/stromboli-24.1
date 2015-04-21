
#ifndef _RTNET_INTERNAL_H_
#define _RTNET_INTERNAL_H_

#include <rtnet.h>

extern int rtnet_testing;
extern int rt_local_addr;


void init_rtnet(void);
int rt_net_dev_init(void);

void rt_inet_proto_init(void);
void rt_ip_init(void);
void rt_arp_init(void);
void rt_icmp_init(void);
void rt_udp_init(void);
void rt_ip_rt_init(void);
void rt_echo_init(void);

void rtskb_init(void);

/* internal functions */

struct rtsocket *rtsocket_lookup(int s);

extern struct rtsocket *rt_ip_sockets;
void rt_inet_add_protocol(struct rtsocket *prot);
struct rtsocket * rt_inet_get_protocol(unsigned char prot);
int rt_inet_del_protocol(struct rtsocket *prot);

int rt_udp_add_socket(struct rtsocket *s);

struct rtsocket *rtsocket_alloc(void);

struct rtsocket *rt_udp_socket_new(void);

int rt_deliver_drop(struct rtsocket *sk,struct rtskb *skb);

int memcpy_fromkerneliovec(unsigned char *kdata,struct iovec *iov,int len);

void rtnet_chrdev_init(void);
void rtnet_chrdev_cleanup(void);



#endif

