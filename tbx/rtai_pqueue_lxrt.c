/*
COPYRIGHT (C) 2003  Paolo Mantegazza (mantegazza@aero.polimi.it),

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <rtai_pqueue.h>

#define MODULE_NAME "rtai_mq_lxrt"

static struct rt_fun_entry rt_mq_fun[] __attribute__ ((__unused__));

static struct rt_fun_entry rt_mq_fun[] = {
	{ UR1(1, 5) | UR2(4, 6), mq_open },			//   0
	{ UW1(2, 3) | UW2(4, 0), mq_receive },			//   1
	{ UR1(2, 3), mq_send },					//   2
	{ 1, mq_close },					//   3
	{ UW1(2, 3), mq_getattr },				//   4
	{ UR1(2, 4) | UW1(3, 4), mq_setattr },			//   5
	{ UR1(2, 3), mq_notify },				//   6
	{ UR1(1, 2), mq_unlink },				//   7
	{ UW1(2, 3) | UW2(4, 0) | UR1(5, 6), mq_timedreceive },	//   8
	{ UR1(2, 3) | UR2(5, 6), mq_timedsend }			//   9
};

int init_module(void)
{
        if (set_rt_fun_ext_index(rt_mq_fun, MQIDX)) {
                rt_printk("%d is a wrong index module for lxrt.\n", MQIDX);
                return -EACCES;
        }
        printk("%s: loaded.\n",MODULE_NAME);
        return(0);
}

void cleanup_module(void)
{
        reset_rt_fun_ext_index(rt_mq_fun, MQIDX);
        printk("%s: unloaded.\n", MODULE_NAME);
}
