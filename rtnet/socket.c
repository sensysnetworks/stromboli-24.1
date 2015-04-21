/*
rtnet/socket.c - sockets implementation for rtnet
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

#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

//#include <asm/uaccess.h>

#ifdef CONFIG_RTAI_RTNET_TASK
#include <rtai_sched.h>
#endif

#if 0
/* these functions are not implemented yet. */

int getsockname(int s,struct sockaddr *name,int *namelen);
int getsockopt(int s,int level,int optname,void *optval,int *optlen);
int setsockopt(int s,int level,int optname,const void *optval,int optlen);

struct protoent *getprotoent(void);
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int proto);
void setprotoent(int stayopen);
void endprotoent(void);
#endif

void rt_tasks_start(void);
void rt_tasks_stop(void);

int socket_fd = 1;
#define new_fd()	(socket_fd++)

int rt_iovec_len(struct iovec *iov,int iovlen);

static struct rtsocket rtsockets[RTNET_NUM_SOCKETS];
static struct rtsocket *rtsockets_free;
static struct rtsocket *sock_list;


int rt_socket_callback(int s,int (*func)(int,void *),void *arg)
{
	struct rtsocket *sock=rtsocket_lookup(s);

	sock->private=arg;
	sock->wakeup=func;

	return 0;
}

int rt_close(int s)
{
	struct rtsocket *sock=rtsocket_lookup(s);

	if(!sock)return -ENOTSOCK;

	sock->ops->close(sock,0);

	sock_list->next=sock;
	sock_list=sock;

	return 0;
}

int rt_shutdown(int s,int how)
{
	struct rtsocket *sock=rtsocket_lookup(s);

	if(!sock)return -ENOTSOCK;

	/* XXX */
	return -ENOTCONN;
}


int rt_accept(int s,struct sockaddr *addr,int *addrlen)
{
	return -EOPNOTSUPP;
}

int rt_bind(int s,struct sockaddr *my_addr,int addrlen)
{
	struct rtsocket *sock=rtsocket_lookup(s);

	if(!sock)return -ENOTSOCK;

	return sock->ops->bind(sock,my_addr,addrlen);
}

int rt_connect(int s,struct sockaddr *serv_addr,int addrlen)
{
	struct rtsocket *sock=rtsocket_lookup(s);

	if(!sock)return -ENOTSOCK;

	return sock->ops->connect(sock,serv_addr,addrlen);
}

int rt_listen(int s,int backlog)
{
	struct rtsocket *sock=rtsocket_lookup(s);

	if(!sock)return -ENOTSOCK;

	return sock->ops->listen(sock,backlog);
}

int rt_recv(int s,void *buf,int len,unsigned int flags)
{
	return rt_recvfrom(s,buf,len,flags,NULL,NULL);
}

int rt_recvfrom(int s,void *buf,int len,unsigned int flags,
		struct sockaddr *from,int *fromlen)
{
	struct rtsocket *sock=rtsocket_lookup(s);
	struct msghdr msg;
	struct iovec iov;
	int err;

	if(!sock)return -ENOTSOCK;

	iov.iov_base=buf;
	iov.iov_len=len;
	msg.msg_name=from;
	msg.msg_iov=&iov;
	msg.msg_iovlen=1;
	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_name=from;
	msg.msg_namelen=MAX_SOCK_ADDR;

#if 0
	if(sock->file->f_flags & O_NONBLOCK)
		flags |= MSG_DONTWAIT;
#endif
	msg.msg_flags=flags;	/* XXX eh? */

	err = sock->ops->recvmsg(sock,&msg,len);

	if(err>=0 && fromlen != NULL){
		*fromlen=msg.msg_namelen;
	}

	return err;
}

int rt_recvmsg(int s,struct msghdr *msg,unsigned int flags)
{
	struct rtsocket *sock=rtsocket_lookup(s);
	int len;

	if(!sock)return -ENOTSOCK;

	len=rt_iovec_len(msg->msg_iov,msg->msg_iovlen);

	return sock->ops->recvmsg(sock,msg,len);
}

#if 0
int rt_send(int s,const void *msg,int len,unsigned int flags)
{
	return rt_sendto(s,msg,len,flags,NULL,0);
}
#endif

int rt_sendmsg(int s,const struct msghdr *msg,unsigned int flags)
{
	struct rtsocket *sock=rtsocket_lookup(s);
	int len;

	if(!sock)return -ENOTSOCK;

	len=rt_iovec_len(msg->msg_iov,msg->msg_iovlen);

	return sock->ops->sendmsg(sock,msg,len);
}

int rt_sendto(int s,const void *buf,int len,unsigned int flags,
		struct sockaddr *to,int tolen)
{
	struct rtsocket *sock=rtsocket_lookup(s);
	struct msghdr msg;
	struct iovec iov;

	if(!sock)return -ENOTSOCK;

	iov.iov_base=(void *)buf;
	iov.iov_len=len;
	msg.msg_name=NULL;
	msg.msg_iov=&iov;
	msg.msg_iovlen=1;
	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_name=to;
	msg.msg_namelen=tolen;

#if 0
	if(sock->file->f_flags & O_NONBLOCK)
		flags |= MSG_DONTWAIT;
#endif
	msg.msg_flags=flags;	/* XXX eh? */

	return sock->ops->sendmsg(sock,&msg,len);
}

int rt_socket(int domain,int type,int protocol)
{
	struct rtsocket *sock;

	if(domain!=AF_INET || type!=SOCK_DGRAM)
		return -EINVAL;
	protocol=IPPROTO_UDP;

	sock=rt_udp_socket_new();

	sock->next=sock_list;
	sock_list=sock;

	sock->fd=new_fd();

	return sock->fd;
}


/* XXX these need to be somewhere more appropriate */

struct rtsocket *rtsocket_alloc(void)
{
	struct rtsocket *sock;
	
	sock=rtsockets_free;
	if(sock){
		rtsockets_free=sock->next;

		memset(sock,0,sizeof(*sock));
		sock->next=NULL;
		sock->use_count=1;

		rtskb_queue_head_init(&sock->incoming);
	}else{
		rt_printk("no rt sockets\n");
	}

	return sock;
}

void rt_socket_init(void)
{
	int i;

#if 0
	rtsockets=kmalloc(sizeof(struct rtsocket)*N_RT_SOCKETS,GFP_KERNEL);
#endif
	for(i=0;i<N_RT_SOCKETS-1;i++){
		(rtsockets+i)->next=rtsockets+i+1;
	}
	rtsockets_free=rtsockets;
}

void rtsocket_free(struct rtsocket *sock)
{
	sock->use_count--;

	if(!sock->use_count){
		sock->next=rtsockets_free;
		rtsockets_free=sock;
	}
}

int rt_sock_register(struct net_proto_family *ops)
{
	return 0;
}

int rt_deliver_drop(struct rtsocket *sk,struct rtskb *skb)
{
	rt_printk("dropping in rt_deliver_drop()\n");

	kfree_rtskb(skb);

	return 0;
}

int rt_iovec_len(struct iovec *iov,int iovlen)
{
	int i,len=0;

	for(i=0;i<iovlen;i++){
		len+=iov[i].iov_len;
	}

	return len;
}

struct rtsocket *rtsocket_lookup(int s)
{
	struct rtsocket *sock;

	for(sock=sock_list;sock;sock=sock->next){
		if(sock->fd==s)return sock;
	}
	return NULL;
}



void rt_socket_init(void);
#ifdef CONFIG_RTAI_RTNET_TASK
static void rt_rxtask_init(void);
static void rt_rxtask_cleanup(void);
#else
#define rt_rxtask_init()
#define rt_rxtask_cleanup()
#endif

#define RTNET_MAJOR 60

void init_rtnet(void)
{
	printk("Real-time networking\n");

	rtskb_init();
	rt_socket_init();

	rt_net_dev_init();
	rt_inet_proto_init();

	rtnet_chrdev_init();

	rt_rxtask_init();
}

#ifdef CONFIG_RTAI_RTNET_TASK

static RT_TASK rxtask;
static SEM rxsem;

static void do_rxtask(int x)
{
	while(1){
		rt_sem_wait(&rxsem);
		rtnet_bh();
	}
}

static void rt_rxtask_init(void)
{
	rt_sem_init(&rxsem,0);
	rt_task_init(&rxtask,&do_rxtask,0,4096,RTNET_TASK_PRIORITY,
		0,0);
	//rt_set_runnable_on_cpus(&rxtask,3);
	rt_task_resume(&rxtask);
}

static void rt_rxtask_cleanup(void)
{
	rt_task_delete(&rxtask);

}

void rtnet_mark_rxtask(void)
{
	rt_sem_signal(&rxsem);
}
#endif

#ifdef MODULE
int init_module(void)
{
	init_rtnet();

	return 0;
}

void cleanup_module(void)
{
	rt_rxtask_cleanup();

	rtnet_chrdev_cleanup();
}
#endif


/* emulate linux interrupts for RTAI */

struct rt_irq_crap{
	void *dev;
	void (*irq_func)(int irq,void *dev,struct pt_regs *regs);
};

static struct pt_regs bogus_regs;
static int rt_irq;
static struct rt_irq_crap irq_struct;

static void rtai_interrupt(void)
{
	rt_disable_irq(rt_irq);
	irq_struct.irq_func(rt_irq,irq_struct.dev,&bogus_regs);
	rt_enable_irq(rt_irq);
}

int rtnet_request_irq(unsigned int irq,void (*handler)(int,void *,struct pt_regs *),
		unsigned long flags, const char *device, void *dev_id)
{
	irq_struct.dev=dev_id;
	irq_struct.irq_func=handler;

	rt_request_global_irq(irq,&rtai_interrupt);
	rt_enable_irq(irq);

	return 0;
}

void rtnet_free_irq(unsigned int irq, void *dev_id)
{
	rt_disable_irq(irq);
	rt_free_global_irq(irq);
}


