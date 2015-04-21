/*
Copyright 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)
          2000  Steve Papacharalombous <stevep@zentropix.com>
	  2001  David Schleef <ds@schleef.org>

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

/* 
ACKNOWLEDGMENTS: 
- Steve Papacharalambous (stevep@zentropix.com) has contributed an informative 
  proc filesystem procedure.
*/


#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>

//#include <linux/version.h>
//#include <linux/sched.h>
//#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/system.h>
//#include <asm/hw_irq.h>
//#include <asm/smp.h>
//#include <asm/io.h>
//#include <asm/bitops.h>
//#include <asm/atomic.h>
//#include <asm/apicdef.h>

#include <rtai.h>

/* stuff that should go in an rtai internal header file */
#define rtai_mounted 1
#define NR_SRQS 16

extern struct cpu_own_irq_handling {
	volatile unsigned long dest_status;
	void (*handler)(void);
} cpu_own_irq[];
extern void (*global_irq_handler[])(void);

typedef struct rt_irq_state_struct rt_irq_state;

struct rt_irq_state_struct {
        int                     irq;
        int                     type;
        int                     priority;
        rt_irq_state            *pnext;
        rt_irq_state            *pprev;
        hw_irq_controller       *handler;
        unsigned int            status;
        int                     (*action)(int,void *,struct pt_regs *);
        void *dev_id;
        unsigned long           arg;
};

extern rt_irq_state rt_irq_desc[];
extern rt_irq_state rt_timer_desc;
extern rt_irq_state rt_srq_desc[];

/* */

int rtai_proc_register(void);
void rtai_proc_unregister(void);

struct proc_dir_entry *rtai_proc_root = NULL;

static int rtai_read_rtai(char *page, char **start, off_t off, int count,
                          int *eof, void *data)
{
	PROC_PRINT_VARS;
	int i;

	PROC_PRINT("\nRTAI Real Time Kernel, Version: %s\n\n", RTAI_VERSION);
	PROC_PRINT("    RTAI mount count: %d\n", rtai_mounted);
#ifdef CONFIG_SMP
	PROC_PRINT("    APIC Frequency: %d\n", tuned.apic_freq);
	PROC_PRINT("    APIC Latency: %d ns\n", LATENCY_APIC);
	PROC_PRINT("    APIC Setup: %d ns\n", SETUP_TIME_APIC);
#endif
#ifdef CONFIG_PPC
	//PROC_PRINT("    APIC Frequency: %d\n", FREQ_DECR);
	//PROC_PRINT("    APIC Latency: %d ns\n", LATENCY_DECR);
	//PROC_PRINT("    APIC Setup: %d ns\n", SETUP_TIME_DECR);
#endif
	PROC_PRINT("\nGlobal irqs used by RTAI: \n");
	for (i = 0; i < NR_IRQS; i++) {
#ifdef CONFIG_PPC
		if (rt_irq_desc[i].action) {
			PROC_PRINT("%d ", i);
		}
#else
		if (global_irq_handler[i]) {
			PROC_PRINT("%d ", i);
		}
#endif
	}
	PROC_PRINT("\nCpu_Own irqs used by RTAI: \n");
#ifndef CONFIG_PPC
#ifndef CONFIG_UCLINUX
	for (i = 0; i < NR_CPU_OWN_IRQS; i++) {
		if (rt_irq_desc[i].action) {
			PROC_PRINT("%d ", i);
		}
	}
#endif
#endif
	PROC_PRINT("\nRTAI sysreqs in use: \n");
	for (i = 0; i < NR_SRQS; i++) {
		if (rt_srq_desc[i].action) {
			PROC_PRINT("%d ", i);
		}
	}
	PROC_PRINT("\n\n");

	PROC_PRINT_DONE;
}       /* End function - rtai_read_rtai */

int rtai_proc_register(void)
{
	struct proc_dir_entry *ent;

	rtai_proc_root = create_proc_entry("rtai", S_IFDIR, 0);
	if (!rtai_proc_root) {
		printk("Unable to initialize /proc/rtai\n");
		return(-1);
	}
	rtai_proc_root->owner = THIS_MODULE;
	ent = create_proc_entry("rtai", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
	if (!ent) {
		printk("Unable to initialize /proc/rtai/rtai\n");
		return(-1);
	}
	ent->read_proc = rtai_read_rtai;
	return(0);
}
/* End function - rtai_proc_register */


void rtai_proc_unregister(void)
{
        remove_proc_entry("rtai", rtai_proc_root);
        remove_proc_entry("rtai", 0);
}       /* End function - rtai_proc_unregister */

