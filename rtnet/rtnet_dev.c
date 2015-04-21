/*
rtnet/rtnet_dev.c - implement char device for user space communication
Copyright (C) 1999    Lineo, Inc
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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/kmod.h>
#include <linux/miscdevice.h>

#include <asm/uaccess.h>

#include <rtnet.h>
#include <rtnet_internal.h>


/* /dev/rtnet interface */

static int rtnet_open(struct inode *inode,struct file *file)
{
	return 0;
}

static int rtnet_ioctl(struct inode *inode,struct file *file,unsigned int cmd,unsigned long arg)
{
	struct rtnet_config cfg;
	struct rtnet_device *rtdev;
	int ret;

	if (!suser())return -EPERM;

	ret = copy_from_user(&cfg, (void *)arg, sizeof(cfg));
	if (ret) return -EFAULT;

	rtdev = rt_dev_get(cfg.if_name);
	if (!rtdev){
#ifdef CONFIG_KMOD
		ret = request_module(cfg.if_name);

		/* try again... */
		rtdev = rt_dev_get(cfg.if_name);
#endif

		if(!rtdev){
			printk("invalid interface %s\n", cfg.if_name);
			return -ENODEV;
		}
	}

	switch(cmd){
	case IOC_RT_IFUP:
		rt_dev_open(rtdev);

		rtdev->local_addr = cfg.ip_addr;

		rt_ip_route_add(rtdev,cfg.ip_netaddr,cfg.ip_mask);

		return 0;
	case IOC_RT_IFDOWN:
		rt_dev_close(rtdev);

		return 0;
	case IOC_RT_ROUTE_SOLICIT:
		rt_arp_solicit(rtdev,cfg.ip_addr);

		return 0;
	default:
		return -ENOTTY;
	}
}

static struct file_operations rtnet_fops = {
	ioctl:	rtnet_ioctl,
	open:	rtnet_open,
};

static struct miscdevice rtnet_chr_misc_dev = {
	minor:		RTNET_MINOR,
	name:		"rtnet",
	fops:		&rtnet_fops,
};

void rtnet_chrdev_init(void)
{
	if(misc_register(&rtnet_chr_misc_dev) < 0) {
		printk("rtnet: unable to register rtnet char misc device\n");
	}
}


void rtnet_chrdev_cleanup(void)
{
	misc_deregister(&rtnet_chr_misc_dev);
}

