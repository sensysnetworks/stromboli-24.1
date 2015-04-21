/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/config.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

//#define SCHED_LOCK

#ifdef CONFIG_UCLINUX
#define TIMERTICKS 1000000
#else
#define TIMERTICKS 500000
#endif

#define CMDF0 0

#define ONE_SHOT

static RT_TASK Slow_Task;
static RT_TASK Fast_Task;

static int cpu_used[NR_RT_CPUS];

struct msg_t {
  char task, susres;
  unsigned long flags;
  RTIME time;
};

static void Slow_Thread(int t)
{
        static struct msg_t msg = {'S',};                      
        while (1) {  
		cpu_used[hard_cpu_id()]++;
#ifdef SCHED_LOCK
		rt_sched_lock();
#endif
		while(1) {
                        msg.time = rt_get_cpu_time_ns();
                        msg.susres = 'r'; 
		        rt_global_save_flags(&msg.flags);
                        if(rtf_put(CMDF0, &msg, sizeof(msg))) break;
			rt_task_wait_period();
		};

                rt_busy_sleep(11*TIMERTICKS);

                while(1) {
                        msg.time = rt_get_cpu_time_ns();
                        msg.susres = 's'; 
		        rt_global_save_flags(&msg.flags);
                        if(rtf_put(CMDF0, &msg, sizeof(msg))) break;
			rt_task_wait_period();
		};

		rt_sched_unlock();

                rt_task_wait_period();                                        
        }
}                                        

static void Fast_Thread(int t) 
{                             
        static struct {char task, susres; unsigned long flags; RTIME time;} msg = {'F',};                      

        while (1) {
		cpu_used[hard_cpu_id()]++;
                while(1) {
                        msg.time = rt_get_time_ns();
                        msg.susres = 'r';
#ifndef CONFIG_UCLINUX 
		        rt_global_sti();
#endif
		        rt_global_save_flags(&msg.flags);
                        if(rtf_put(CMDF0, &msg, sizeof(msg))) break;
			rt_task_wait_period();
		}; 

                rt_busy_sleep(2*TIMERTICKS);

                while(1) {
                        msg.time = rt_get_time_ns();
                        msg.susres = 's'; 
		        rt_global_save_flags(&msg.flags);
                        if(rtf_put(CMDF0, &msg, sizeof(msg))) break;
			rt_task_wait_period();
		};

                rt_task_wait_period();                                        
        }                      
}
                                                                                   
int init_module(void)
{                   
        RTIME tick_period;
        RTIME now;                                                                 
	rtf_create_using_bh(CMDF0, 1000*sizeof(struct msg_t), 0);
	rt_task_init_cpuid(&Fast_Task, Fast_Thread, 0, 2000, 0, 0, 0, 0);
	rt_task_init_cpuid(&Slow_Task, Slow_Thread, 0, 2000, 1, 0, 0, 0);

#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	tick_period = 4*start_rt_timer(nano2count(TIMERTICKS));
	now = rt_get_time();
	rt_task_make_periodic(&Fast_Task, now + tick_period,  tick_period);
	rt_task_make_periodic(&Slow_Task, now + tick_period,  6*tick_period);

	return 0;
}

void cleanup_module(void)
{
	int cpuid;
	stop_rt_timer();
	rt_busy_sleep(10000000);
	rtf_destroy(CMDF0);
	rt_task_delete(&Slow_Task);
	rt_task_delete(&Fast_Task);
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
