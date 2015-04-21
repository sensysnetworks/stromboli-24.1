/*
COPYRIGHT (C) 2001 Wolfgang Grandegger <wg@denx.de>

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
 * This example shows how to handle RTAI interrupts from CPM timers.
 * The CPM timer handling is described in the MPC860 User's Manual 
 * in Paragraph 18 or the MPC8260 User's Manual in Paragraph 17.
 */

#include <linux/types.h>
#include <linux/config.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/irq.h>

#ifdef CONFIG_8xx
#include <asm/8xx_immap.h>
#include <asm/mpc8xx.h>
#else
#ifdef CONFIG_8260
#include <asm/immap_8260.h>
#include <asm/mpc8260.h>
#else
#error "Your platform is not supported!" 
#endif
#endif

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>


#undef DEBUG

#ifdef DEBUG
# define debugk(fmt,args...)	printk(fmt ,##args)
#else
# define debugk(fmt,args...)
#endif


MODULE_AUTHOR("Wolfgang Grandegger (wg@denx.de)");
MODULE_DESCRIPTION("RTAI Timer Demo");

int hz = 1;
MODULE_PARM(hz, "i");
MODULE_PARM_DESC(freq, "Timer frequency in Hz (default=1)");

int print = 10;
MODULE_PARM(print, "i");
MODULE_PARM_DESC(print, "Print message every N interrupts (default=10)");

MODULE_LICENSE("GPL");


#ifdef CONFIG_8xx
# define CPMVEC_TIMER2	((ushort)0x12)
# define IRQ_TIMER2	(CPM_IRQ_OFFSET + CPMVEC_TIMER2)
static cpmtimer8xx_t *t;
#else /* CONFIG_8260 */
# define SIU_INT_TIMER2	((uint)0x0d)
# define IRQ_TIMER2	SIU_INT_TIMER2
static cpmtimer8260_t *t;
#endif

static RT_TASK thread;
static unsigned long irq_count = 0;

#define STACK_SIZE 3000

#undef USE_ISR_WITH_ARG

#ifdef USE_ISR_WITH_ARG
static int cpm_irq_handler (unsigned int irq, unsigned long data)
#else
static void cpm_irq_handler (unsigned int irq)
#endif
{
#ifdef USE_ISR_WITH_ARG
	unsigned long *pirq_count = (unsigned long *)data;
#else
	unsigned long *pirq_count = &irq_count;
#endif
	debugk("\nIRQ=%d TER2=0x%x\n\n", irq, t->cpmt_ter2);
	t->cpmt_ter2 |= 0x3;
	(*pirq_count)++;
	rt_task_resume(&thread); /* Wakeup monitor task */
	rt_unmask_irq(irq);
#ifdef USE_ISR_WITH_ARG
	return 0;
#endif
}

static void moni_task(int irq) {

	printk ("Monitor task to handle IRQ %d started...\n", irq);
	while(1) {
		rt_task_suspend(&thread);
		if (print && ((irq_count % print) == 0)) {
			printk ("IRQ count: %ld\n", irq_count);
		}
	}
}

int init_module (void)
{
	immap_t *immap =  (immap_t *)IMAP_ADDR;
	t = &immap->im_cpmtimer;

	printk("Run CPM Timer 2 with %d Hz. You can watch the\n", hz);
	printk("counter with the command: $ cat /proc/rtai/rtai\n"); 

	/* Initialize task for monitoring timer interrupts */
	rt_task_init(&thread, moni_task, IRQ_TIMER2, STACK_SIZE, 0, 0, 0);
	rt_task_resume(&thread);

	/* Register handler for CPM Timer 2 interrupts */ 
#ifdef USE_ISR_WITH_ARG
	rt_request_global_irq_ext(IRQ_TIMER2, cpm_irq_handler, 
				  (unsigned long)&irq_count);
#else
	rt_request_global_irq(IRQ_TIMER2, cpm_irq_handler);
#endif

	/* Setup CPM Timer 2 interrupts */
#ifdef CONFIG_8xx
	t->cpmt_tgcr  = 0;
#else /* CONFIG_8260 */
	t->cpmt_tgcr1 = 0;
#endif
	t->cpmt_tmr2  = 0xfa1c;
#ifdef CONFIG_8xx
	t->cpmt_trr2  = CPU_FREQ / hz / 250;
#else
	t->cpmt_trr2  = CPU_FREQ / 4 / hz / 250;
#endif
	t->cpmt_ter2  = 0xffff;

	rt_enable_irq(IRQ_TIMER2);

#ifdef CONFIG_8xx
	t->cpmt_tgcr  = 0x0010;
#else /* CONFIG_8260 */
	t->cpmt_tgcr1 = 0x0010;
#endif
	return 0;
}

void cleanup_module (void)
{
#ifdef CONFIG_8xx
	t->cpmt_tgcr  = 0;
#else /* CONFIG_8260 */
	t->cpmt_tgcr1 = 0;
#endif
	rt_disable_irq(IRQ_TIMER2);
	rt_free_global_irq(IRQ_TIMER2);
	rt_task_delete(&thread);
}
