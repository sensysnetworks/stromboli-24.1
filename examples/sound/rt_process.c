
#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/io.h>

#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>

#include "pcsp_tables.h"

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define ONESHOT

#define TIMER_TO_CPU 3 // < 0 || > 1 to maintain a symmetric processed timer.

#define TICK_PERIOD 25000	/* 40 khz */
#define DIVISOR 5

#define RUNNABLE_ON_CPUS 3  // 1: on cpu 0 only, 2: on cpu 1 only, 3: on any.
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define STACK_SIZE 4000

static RT_TASK thread;

static int cpu_used[NR_RT_CPUS];

static unsigned char vl_tab[256];

static int port61;

static void intr_handler(int t)
{
	char data;
	int go=0;
	int divisor = DIVISOR;

	while(1) {
		if (!(--divisor)) {
			divisor = DIVISOR;
			cpu_used[hard_cpu_id()]++;
			if (!rtf_get(0, &data, 1) > 0) {
				go=0;
			}else{
				go=1;
			}
		}
		if(go){
			outb(port61,0x61);
			outb(port61^1,0x61);
			outb(vl_tab[((unsigned int)data)&0xff], 0x42);
		}

		rt_task_wait_period();
	}
}

static void pcsp_calc_vol(int volume)
{
	int i,j;

	for(i=0;i<256; i++){
		//vl_tab[i]=(volume*ulaw[i])>>8;
		j=i;
		if(j>128)j=382-j;
		vl_tab[i]=1+((volume*ulaw[i])>>8);
	}
}


int init_module(void)
{
	RTIME now, tick_period;

	outb_p(0x92, 0x43); /* binary, mode1, LSB only, ch 2 */

	/* You can make this bigger, but then you start to get
	 * clipping, which sounds bad.  29 is good */
	pcsp_calc_vol(29);

	port61 = inb(0x61) | 0x3;

	rtf_create(0, 2000);
	rt_task_init(&thread, intr_handler, 0, STACK_SIZE, 0, 0, 0);
	rt_set_runnable_on_cpus(&thread, RUN_ON_CPUS);
#ifdef ONESHOT
	rt_set_oneshot_mode();
#endif
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_TO_CPU);
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	now = rt_get_time() + 10*tick_period;
	rt_task_make_periodic(&thread, now, tick_period);

	return 0;
}


void cleanup_module(void)
{
	int cpuid;
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	stop_rt_timer();
	rt_busy_sleep(10000000);
	rt_task_delete(&thread);
	rtf_destroy(0);
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
