/*
COPYRIGHT (C) 2002 Paolo Mantegazza (mantegazza@aero.polimi.it)

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
- Steve Papacharalambous (stevep@zentropix.com) has contributed a very 
  informative proc filesystem procedure.
- Stefano Picerno (stefanopp@libero.it) for suggesting a simple fix to
  distinguish a timeout from an abnormal retrun in timed sem waits.
- Geoffrey Martin (gmartin@altersys.com) for a fix to functions with timeouts.
- Michael D. Kralka (michael.kralka@kvs.com) for a fix to rt_change_priority.
*/


#define USE_RTAI_TASKS

#define ALLOW_RR

#define ONE_SHOT	0
#define PREEMPT_ALWAYS	0

#define cpu_present_map cpu_online_map

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/timex.h>
#include <linux/sched.h>
#include <linux/irq.h>

#include <asm/param.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <asm/hw_irq.h>

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>
static int errno;

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif

#if defined(CONFIG_RTAI_DYN_MM) || defined(CONFIG_RTAI_DYN_MM_MODULE)
//#include <rt_mem_mgr.h>
#define sched_malloc(size)		rt_malloc((size))
#define sched_free(adr)			rt_free((adr))
#if defined(CONFIG_RTAI_DYN_MM_MODULE)
#define sched_mem_init()
#define sched_mem_end()
#else
#define sched_mem_init() \
	{ if(rt_mem_init() != 0) { \
                printk("Failed to allocate memory for task stack(s)\n"); \
                return(-ENOMEM); \
        } }
#define sched_mem_end()			rt_mem_end()
#endif
#define call_exit_handlers(task)	__call_exit_handlers(task)
#define set_exit_handler(task, fun, arg1, arg2)	__set_exit_handler(task, fun, arg1, arg2)
#else
#define sched_malloc(size)		kmalloc((size), GFP_KERNEL)
#define sched_free(adr)			kfree((adr))
#define sched_mem_init()
#define sched_mem_end()
#define call_exit_handlers(task)
#define set_exit_handler(task, fun, arg1, arg2)
#endif

#define INTERFACE_TO_LINUX
#include <rtai/version.h>
#include <rtai.h>
#include <asm/rtai_sched.h>
#include <asm/rtai_lxrt.h>
#if RTAI_VERSION_CODE < RTAI_VERSION(24,1,10)
#include "rtai_sched.h"
#else
#include <rtai_sched.h>
#endif

MODULE_LICENSE("GPL");

#define RT_SEM_MAGIC 0xaabcdeff

#define SEM_ERR (0xFfff)

#define MSG_ERR ((RT_TASK *)0xFfff)

#define NOTHING ((void *)0)

#define SOMETHING ((void *)1)

#ifdef CONFIG_PROC_FS
// proc filesystem additions.
static int rtai_proc_sched_register(void);
static void rtai_proc_sched_unregister(void);
// End of proc filesystem additions. 
#endif

RT_TASK rt_smp_linux_task[NR_RT_CPUS];

static int sched_rqsted[NR_RT_CPUS];

static int rt_smp_linux_cr0[NR_RT_CPUS];

static RT_TASK *rt_smp_current[NR_RT_CPUS];

static RT_TASK *rt_smp_fpu_task[NR_RT_CPUS];

static RTIME rt_smp_time_h[NR_RT_CPUS];

static int rt_smp_half_tick[NR_RT_CPUS];

static int rt_smp_oneshot_timer[NR_RT_CPUS];

static int rt_smp_oneshot_running[NR_RT_CPUS];

static int rt_smp_shot_fired[NR_RT_CPUS];

static int rt_smp_preempt_always[NR_RT_CPUS];

static struct rt_times *linux_times;

static RT_TASK *wdog_task[NR_RT_CPUS];

#define DECLARE_RT_CURRENT int cpuid; RT_TASK *rt_current

#define RT_CURRENT rt_smp_current[hard_cpu_id()]

#define ASSIGN_RT_CURRENT rt_current = rt_smp_current[cpuid = hard_cpu_id()]

#define fpu_task (rt_smp_fpu_task[cpuid])

#define rt_linux_task (rt_smp_linux_task[cpuid])

#define rt_time_h (rt_smp_time_h[cpuid])

#define rt_half_tick (rt_smp_half_tick[cpuid])

#define oneshot_timer (rt_smp_oneshot_timer[cpuid])

#define oneshot_running (rt_smp_oneshot_running[cpuid])

#define oneshot_timer_cpuid (rt_smp_oneshot_timer[hard_cpu_id()])

#define shot_fired (rt_smp_shot_fired[cpuid])

#define preempt_always (rt_smp_preempt_always[cpuid])

#define rt_times (rt_smp_times[cpuid])

#define linux_cr0 (rt_smp_linux_cr0[cpuid])

#define SEMHLF 0x0000FFFF
#define RPCHLF 0xFFFF0000
#define RPCINC 0x00010000

#define MAX_SRQ  64
static struct { int srq, in, out; void *mp[MAX_SRQ]; } frstk_srq;

#ifdef CONFIG_SMP

#define SCHED_IPI     RTAI_3_IPI
#define SCHED_VECTOR  RTAI_3_VECTOR

#define TIMER_FREQ FREQ_APIC
#define TIMER_LATENCY LATENCY_APIC
#define TIMER_SETUP_TIME SETUP_TIME_APIC
#define ONESHOT_SPAN (0x7FFFFFFFLL*(CPU_FREQ/TIMER_FREQ))

#ifdef CONFIG_RTAI_ADEOS
#define set_timer_chip(delay) \
	{ \
	        unsigned long flags; \
		arti_hw_lock(flags); \
		apic_read(APIC_TMICT); \
		apic_write(APIC_TMICT, (delay)); \
		arti_hw_unlock(flags); \
	}
#else /* !CONFIG_RTAI_ADEOS */
#define set_timer_chip(delay) \
	{ \
		apic_read(APIC_TMICT); \
		apic_write(APIC_TMICT, (delay)); \
	}
#endif /* CONFIG_RTAI_ADEOS */

#define update_linux_timer()

extern void broadcast_to_local_timers(int irq,void *dev_id,struct pt_regs *regs);
#define BROADCAST_TO_LOCAL_TIMERS() broadcast_to_local_timers(0,NULL,NULL)

static inline void send_sched_ipi(unsigned long dest)
{
#ifdef CONFIG_RTAI_ADEOS
        unsigned long flags;
	arti_hw_lock(flags);
#endif /* CONFIG_RTAI_ADEOS */
	apic_wait_icr_idle();
	apic_write_around(APIC_ICR2, SET_APIC_DEST_FIELD(dest));
	apic_write_around(APIC_ICR, APIC_DEST_LOGICAL | SCHED_VECTOR);
#ifdef CONFIG_RTAI_ADEOS
	arti_hw_unlock(flags);
#endif /* CONFIG_RTAI_ADEOS */
}

#define RT_SCHEDULE_MAP(schedmap) \
	do { if (schedmap) send_sched_ipi(schedmap); } while (0)

#define RT_SCHEDULE_MAP_BOTH(schedmap) \
	do { RT_SCHEDULE_MAP(schedmap); rt_schedule(); } while (0)

#define RT_SCHEDULE(task, cpuid) \
	do { \
		if ((task)->runnable_on_cpus != (cpuid)) { \
			send_sched_ipi(1 << (task)->runnable_on_cpus); \
		} else { \
			rt_schedule(); \
		} \
	} while (0)

#define RT_SCHEDULE_BOTH(task, cpuid) \
	{ \
		if ((task)->runnable_on_cpus != (cpuid)) { \
			send_sched_ipi(1 << (task)->runnable_on_cpus); \
		} \
		rt_schedule(); \
	}

#define rt_request_sched_ipi()  rt_request_cpu_own_irq(SCHED_IPI, rt_schedule);

#define rt_free_sched_ipi()     rt_free_cpu_own_irq(SCHED_IPI);

static atomic_t scheduling_cpus = ATOMIC_INIT(0);

static inline void sched_get_global_lock(int cpuid)
{
	if (!test_and_set_bit(cpuid, locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus) && !atomic_read(&scheduling_cpus)) {
#ifdef STAGGER
			STAGGER(cpuid);
#endif
		}
	}
	atomic_inc(&scheduling_cpus);
}

static inline void sched_release_global_lock(int cpuid)
{
	if (test_and_clear_bit(cpuid, locked_cpus) && atomic_dec_and_test(&scheduling_cpus)) {
		test_and_clear_bit(31, locked_cpus);
#ifdef STAGGER
			STAGGER(cpuid);
#endif
	}
}

#else

#define TIMER_FREQ FREQ_8254
#define TIMER_LATENCY LATENCY_8254
#define TIMER_SETUP_TIME SETUP_TIME_8254
#define ONESHOT_SPAN (0x7FFF*(CPU_FREQ/TIMER_FREQ))

#define set_timer_chip(delay)  rt_set_timer_delay(delay)

#define update_linux_timer() rt_pend_linux_irq(TIMER_8254_IRQ)

#define BROADCAST_TO_LOCAL_TIMERS()

#define send_sched_ipi(dest)

#define RT_SCHEDULE_MAP_BOTH(schedmap)  rt_schedule()

#define RT_SCHEDULE_MAP(schedmap)       rt_schedule()

#define RT_SCHEDULE(task, cpuid)        rt_schedule()

#define RT_SCHEDULE_BOTH(task, cpuid)   rt_schedule()

#define rt_request_sched_ipi()

#define rt_free_sched_ipi()

#define sched_get_global_lock(cpuid)

#define sched_release_global_lock(cpuid)

#endif

#define BASE_SOFT_PRIORITY 1000000000

#ifdef CONFIG_SMP
#define SWITCH_MEM  0x00000000
#else
#define SWITCH_MEM  0x00000000
#endif

static inline void TASK_SWITCH(struct task_struct *prev, struct task_struct *next, int cpuid)
{
	rthal.switch_mem(prev, next, cpuid | SWITCH_MEM);
	my_switch_to(prev, next, prev);
}

/* ++++++++++++++++++++++++++++++++ TASKS ++++++++++++++++++++++++++++++++++ */

#define TASK_HARDREALTIME  TASK_UNINTERRUPTIBLE

static inline void enq_ready_edf_task(RT_TASK *ready_task)
{
	RT_TASK *task;
	task = rt_smp_linux_task[ready_task->runnable_on_cpus].rnext;
	while (task->policy < 0 && ready_task->period >= task->period) {
		task = task->rnext;
	}
	task->rprev = (ready_task->rprev = task->rprev)->rnext = ready_task;
	ready_task->rnext = task;
}

#undef MAX_SRQ
#define MAX_SRQ (2 << 6)

struct klist_t { volatile int srq, in, out; void *task[MAX_SRQ]; };
static struct klist_t wake_up_srq;

static inline void enq_ready_task(RT_TASK *ready_task)
{
	RT_TASK *task;
	if (ready_task->is_hard || (ready_task->lnxtsk)->state != TASK_HARDREALTIME) {
		task = rt_smp_linux_task[ready_task->runnable_on_cpus].rnext;
		while (ready_task->priority >= task->priority) {
			if ((task = task->rnext)->priority < 0) break;
		}
		task->rprev = (ready_task->rprev = task->rprev)->rnext = ready_task;
		ready_task->rnext = task;
	} else {
		wake_up_srq.task[wake_up_srq.in] = ready_task->lnxtsk;
		wake_up_srq.in = (wake_up_srq.in + 1) & (MAX_SRQ - 1);
		rt_pend_linux_srq(wake_up_srq.srq);
	}
}

static inline int renq_ready_task(RT_TASK *ready_task, int priority)
{
	int retval;
	if ((retval = ready_task->priority != priority)) {
		ready_task->priority = priority;
		if (ready_task->state == READY) {
			(ready_task->rprev)->rnext = ready_task->rnext;
			(ready_task->rnext)->rprev = ready_task->rprev;
			enq_ready_task(ready_task);
		}
	}
	return retval;
}

static inline int renq_current(RT_TASK *rt_current, int priority)
{
	int retval;
	if ((retval = rt_current->priority != priority)) {
		rt_current->priority = priority;
		(rt_current->rprev)->rnext = rt_current->rnext;
		(rt_current->rnext)->rprev = rt_current->rprev;
		enq_ready_task(rt_current);
	}
	return retval;
}

static inline void rem_ready_task(RT_TASK *task)
{
	if (task->state == READY) {
		if (!task->is_hard) {
			struct task_struct *proc;
			(proc = task->lnxtsk)->state = TASK_HARDREALTIME;
			proc->rt_priority = (2*BASE_SOFT_PRIORITY - 1) - task->priority;
		}
		(task->rprev)->rnext = task->rnext;
		(task->rnext)->rprev = task->rprev;
	}
}

static inline void rem_ready_current(RT_TASK *rt_current)
{
	if (!rt_current->is_hard) {
		struct task_struct *proc;
		(proc = rt_current->lnxtsk)->state = TASK_HARDREALTIME;
		proc->rt_priority = (2*BASE_SOFT_PRIORITY - 1) - rt_current->priority;
	}
	(rt_current->rprev)->rnext = rt_current->rnext;
	(rt_current->rnext)->rprev = rt_current->rprev;
}

#define TASK_TO_SCHEDULE() \
	do { prio = (new_task = rt_linux_task.rnext)->priority; } while(0)

static inline void enq_timed_task(RT_TASK *timed_task)
{
	RT_TASK *task;
	if (!timed_task->is_hard) {
		(timed_task->lnxtsk)->state = TASK_HARDREALTIME;
	}
	task = rt_smp_linux_task[timed_task->runnable_on_cpus].tnext;
	while (timed_task->resume_time > task->resume_time) {
		task = task->tnext;
	}
	task->tprev = (timed_task->tprev = task->tprev)->tnext = timed_task;
	timed_task->tnext = task;
}

static inline void wake_up_timed_tasks(int cpuid)
{
	RT_TASK *task;
	task = rt_smp_linux_task[cpuid].tnext;
	while (task->resume_time <= rt_time_h) {
		if ((task->state &= ~(DELAYED | SEMAPHORE | RECEIVE | SEND | RPC | RETURN | MBXSUSP)) == READY) {
			if (task->policy < 0) {
				enq_ready_edf_task(task);
			} else {
				enq_ready_task(task);
			}
		}
		task = task->tnext;
	}
	rt_smp_linux_task[cpuid].tnext = task;
	task->tprev = &rt_smp_linux_task[cpuid];
}

static inline void rem_timed_task(RT_TASK *task)
{
	if ((task->state & DELAYED)) {
		(task->tprev)->tnext = task->tnext;
		(task->tnext)->tprev = task->tprev;
	}
}

static int tasks_per_cpu[NR_RT_CPUS] = { 0, };

int get_min_tasks_cpuid(void)
{
	int i, cpuid, min;
	min =  tasks_per_cpu[cpuid = 0];
	for (i = 1; i < NR_RT_CPUS; i++) {
		if (tasks_per_cpu[i] < min) {
			min = tasks_per_cpu[cpuid = i];
		}
	}
	return cpuid;
}


static void put_current_on_cpu(int cpuid)
{
	current->cpus_allowed = 1 << cpuid;
	while (cpuid != hard_cpu_id()) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(1);
	}
}

int set_rtext(RT_TASK *task, int priority, int uses_fpu, void(*signal)(void), unsigned int cpuid, struct task_struct *relink)
{
	unsigned long flags;

	if (task->magic == RT_TASK_MAGIC || cpuid >= NR_RT_CPUS || priority < 0) {
		return -EINVAL;
	} 
	if (wdog_task[cpuid] && wdog_task[cpuid] != task 
		             && priority == RT_HIGHEST_PRIORITY) {
	    	 rt_printk("Highest priority reserved for RTAI watchdog\n");
		 return -EBUSY;
	}
	task->uses_fpu = uses_fpu ? 1 : 0;
	task->runnable_on_cpus = cpuid;
	(task->stack_bottom = (int *)&task->fpu_reg)[0] = 0;
	task->magic = RT_TASK_MAGIC; 
	task->policy = 0;
	task->owndres = 0;
	task->priority = task->base_priority = priority;
	task->prio_passed_to = 0;
	task->period = 0;
	task->resume_time = RT_TIME_END;
	task->queue.prev = task->queue.next = &(task->queue);      
	task->queue.task = task;
	task->msg_queue.prev = task->msg_queue.next = &(task->msg_queue);      
	task->msg_queue.task = task;    
	task->msg = 0;  
	task->ret_queue.prev = task->ret_queue.next = &(task->ret_queue);
	task->ret_queue.task = NOTHING;
	task->tprev = task->tnext = task->rprev = task->rnext = task;
	task->blocked_on = NOTHING;        
	task->signal = signal;
	memset(task->task_trap_handler, 0, RTAI_NR_TRAPS*sizeof(void *));
	task->tick_queue        = NOTHING;
	task->trap_handler_data = NOTHING;
	task->resync_frame = 0;
	task->ExitHook = 0;
	task->usp_flags = task->usp_flags_mask = task->force_soft = 0;
	task->msg_buf[0] = 0;
	task->exectime[0] = 0;
	task->system_data_ptr = 0;
	atomic_inc((atomic_t *)(tasks_per_cpu + cpuid));
	if (relink) {
		task->suspdepth = task->is_hard = 1;
		task->state = READY | SUSPENDED;
		relink->this_rt_task[0] = task;
		task->lnxtsk = relink;
	} else {
		task->suspdepth = task->is_hard = 0;
		task->state = READY;
		current->this_rt_task[0] = task;
		current->this_rt_task[1] = task->lnxtsk = current;
		put_current_on_cpu(cpuid);
	}
	flags = rt_global_save_flags_and_cli();
	task->next = 0;
	rt_linux_task.prev->next = task;
	task->prev = rt_linux_task.prev;
	rt_linux_task.prev = task;
	rt_global_restore_flags(flags);

	return 0;
}


static void start_stop_kthread(RT_TASK *, void (*)(int), int, int, int, void(*)(void), int);

int rt_kthread_init_cpuid(RT_TASK *task, void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu,
			void(*signal)(void), unsigned int cpuid)
{
	start_stop_kthread(task, rt_thread, data, priority, uses_fpu, signal, cpuid);
	return (int)task->retval;
}


int rt_kthread_init(RT_TASK *task, void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu,
			void(*signal)(void))
{
	return rt_kthread_init_cpuid(task, rt_thread, data, stack_size, priority, 
				 uses_fpu, signal, get_min_tasks_cpuid());
}


#ifdef USE_RTAI_TASKS

static void rt_startup(void(*rt_thread)(int), int data)
{
	extern int rt_task_delete(RT_TASK *);
	rt_global_sti();
	RT_CURRENT->exectime[1] = rdtsc();
	rt_thread(data);
	rt_task_delete(rt_smp_current[hard_cpu_id()]);
}


int rt_task_init_cpuid(RT_TASK *task, void (*rt_thread)(int), int data, int stack_size, int priority, int uses_fpu, void(*signal)(void), unsigned int cpuid)
{
	int *st, i;
	unsigned long flags;

	if (task->magic == RT_TASK_MAGIC || cpuid >= NR_RT_CPUS || priority < 0) {
		return -EINVAL;
	} 
	if (!(st = (int *)sched_malloc(stack_size))) {
		return -ENOMEM;
	}
	if (wdog_task[cpuid] && wdog_task[cpuid] != task 
		             && priority == RT_HIGHEST_PRIORITY) {
	    	 rt_printk("Highest priority reserved for RTAI watchdog\n");
		 return -EBUSY;
	}

	task->bstack = task->stack = (int *)(((unsigned long)st + stack_size - 0x10) & ~0xF);
	task->stack[0] = 0;
	task->uses_fpu = uses_fpu ? 1 : 0;
	task->runnable_on_cpus = cpuid;
	atomic_inc((atomic_t *)(tasks_per_cpu + cpuid));
	*(task->stack_bottom = st) = 0;
	task->magic = RT_TASK_MAGIC; 
	task->policy = 0;
	task->suspdepth = 1;
	task->state = (SUSPENDED | READY);
	task->owndres = 0;
	task->is_hard = 1;
	task->lnxtsk = 0;
	task->priority = task->base_priority = priority;
	task->prio_passed_to = 0;
	task->period = 0;
	task->resume_time = RT_TIME_END;
	task->queue.prev = &(task->queue);      
	task->queue.next = &(task->queue);      
	task->queue.task = task;
	task->msg_queue.prev = &(task->msg_queue);      
	task->msg_queue.next = &(task->msg_queue);      
	task->msg_queue.task = task;    
	task->msg = 0;  
	task->ret_queue.prev = &(task->ret_queue);
	task->ret_queue.next = &(task->ret_queue);
	task->ret_queue.task = NOTHING;
	task->tprev = task->tnext =
	task->rprev = task->rnext = task;
	task->blocked_on = NOTHING;        
	task->signal = signal;
	for (i = 0; i < RTAI_NR_TRAPS; i++) {
		task->task_trap_handler[i] = NULL;
	}
	task->tick_queue        = SOMETHING;
	task->trap_handler_data = NOTHING;
	task->resync_frame = 0;
	task->ExitHook = 0;
	task->exectime[0] = 0;
	task->system_data_ptr = 0;

	init_arch_stack();

	flags = rt_global_save_flags_and_cli();
	task->next = 0;
	rt_linux_task.prev->next = task;
	task->prev = rt_linux_task.prev;
	rt_linux_task.prev = task;
	cpuid = hard_cpu_id();
	init_fp_env();
	rt_global_restore_flags(flags);

	return 0;
}


int rt_task_init(RT_TASK *task, void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu,
			void(*signal)(void))
{
	return rt_task_init_cpuid(task, rt_thread, data, stack_size, priority, 
				 uses_fpu, signal, get_min_tasks_cpuid());
}
#else

int rt_task_init_cpuid(RT_TASK *task, void (*rt_thread)(int), int data, int stack_size, int priority, int uses_fpu, void(*signal)(void), unsigned int cpuid)
{
	return rt_kthread_init_cpuid(task, rt_thread, data, stack_size, priority, uses_fpu, signal, cpuid);
}

int rt_task_init(RT_TASK *task, void (*rt_thread)(int), int data, int stack_size, int priority, int uses_fpu, void(*signal)(void))
{
	return rt_kthread_init(task, rt_thread, data, stack_size, priority, uses_fpu, signal);
}

#endif

void rt_set_runnable_on_cpuid(RT_TASK *task, unsigned int cpuid)
{
	unsigned long flags;
	RT_TASK *linux_task;

	return;

	if (cpuid >= NR_RT_CPUS) {
		cpuid = get_min_tasks_cpuid();
	} 
	flags = rt_global_save_flags_and_cli();
	switch (rt_smp_oneshot_timer[task->runnable_on_cpus] | 
		(rt_smp_oneshot_timer[cpuid] << 1)) {	
                case 1:
                        task->period = llimd(task->period, TIMER_FREQ, tuned.cpu_freq);
                        task->resume_time = llimd(task->resume_time, TIMER_FREQ, tuned.cpu_freq);
                        break;
                case 2:
                        task->period = llimd(task->period, tuned.cpu_freq, TIMER_FREQ);
                        task->resume_time = llimd(task->resume_time, tuned.cpu_freq, TIMER_FREQ);
			break;
	}
	if (!((task->prev)->next = task->next)) {
		rt_smp_linux_task[task->runnable_on_cpus].prev = task->prev;
	} else {
		(task->next)->prev = task->prev;
	}
	task->runnable_on_cpus = cpuid;
	if ((task->state & DELAYED)) {
		(task->tprev)->tnext = task->tnext;
		(task->tnext)->tprev = task->tprev;
		enq_timed_task(task);
	}
	task->next = 0;
	(linux_task = rt_smp_linux_task + cpuid)->prev->next = task;
	task->prev = linux_task->prev;
	linux_task->prev = task;
	rt_global_restore_flags(flags);
}


void rt_set_runnable_on_cpus(RT_TASK *task, unsigned int run_on_cpus)
{
	int cpuid;

	return;

	run_on_cpus &= cpu_present_map;
	cpuid = get_min_tasks_cpuid();
	if (!test_bit(cpuid, &run_on_cpus)) {
		cpuid = ffnz(run_on_cpus);
	}
	rt_set_runnable_on_cpuid(task, cpuid);
}


int rt_check_current_stack(void)
{
	DECLARE_RT_CURRENT;
	char *sp;

	if ((rt_current = rt_smp_current[cpuid = hard_cpu_id()]) != &rt_linux_task) {
		sp = get_stack_pointer();
		return (sp - (char *)(rt_current->stack_bottom));
	} else {
		return -0x7FFFFFFF;
	}
}


void rt_set_sched_policy(RT_TASK *task, int policy, int rr_quantum_ns)
{
	if ((task->policy = policy ? 1 : 0)) {
		task->rr_quantum = nano2count_cpuid(rr_quantum_ns, task->runnable_on_cpus);
		if ((task->rr_quantum & 0xF0000000) || !task->rr_quantum) {
			task->rr_quantum = rt_smp_times[task->runnable_on_cpus].linux_tick;
		}
		task->rr_remaining = task->rr_quantum;
		task->yield_time = 0;
	}
}


#ifdef ALLOW_RR
#define RR_YIELD() \
if (rt_current->policy > 0) { \
	rt_current->rr_remaining = rt_current->yield_time - rt_times.tick_time; \
	if (rt_current->rr_remaining <= 0) { \
		rt_current->rr_remaining = rt_current->rr_quantum; \
		if (rt_current->state == READY) { \
			RT_TASK *task; \
			task = rt_current->rnext; \
			while (rt_current->priority == task->priority) { \
				task = task->rnext; \
			} \
			if (task != rt_current->rnext) { \
				(rt_current->rprev)->rnext = rt_current->rnext; \
				(rt_current->rnext)->rprev = rt_current->rprev; \
				task->rprev = (rt_current->rprev = task->rprev)->rnext = rt_current; \
				rt_current->rnext = task; \
			} \
		} \
	} \
}

#define RR_SETYT() \
	if (new_task->policy > 0) { \
		new_task->yield_time = rt_time_h + new_task->rr_remaining; \
	}

#define RR_SPREMP() \
	if (new_task->policy > 0) { \
		preempt = 1; \
		if (new_task->yield_time < intr_time) { \
			intr_time = new_task->yield_time; \
		} \
	} else { \
		preempt = 0; \
	}

#define RR_TPREMP() \
	if (new_task->policy > 0) { \
		preempt = 1; \
		if (new_task->yield_time < rt_times.intr_time) { \
			rt_times.intr_time = new_task->yield_time; \
		} \
	} else { \
		preempt = preempt_always || prio == RT_LINUX_PRIORITY; \
	}

#else
#define RR_YIELD()

#define RR_SETYT()

#define RR_SPREMP() \
do { preempt = 0; } while (0)

#define RR_TPREMP() \
do { preempt = preempt_always || prio == RT_LINUX_PRIORITY; } while (0)
#endif

extern volatile unsigned long lxrt_hrt_flags;
extern unsigned long linux_save_flags_and_cli_cpuid(int);
extern void rtai_just_copy_back(unsigned long, int);

#if RTAI_VERSION_CODE < RTAI_VERSION(24,1,9)
static inline void restore_fpenv_lxrt(struct task_struct *tsk)
{
	if (cpu_has_fxsr) {
		__asm__ __volatile__ ("clts; fxrstor %0": : "m" (tsk->thread.i387.fxsave));
	} else {
		__asm__ __volatile__ ("clts; frstor %0": : "m" (tsk->thread.i387.fsave));
	}
}
#endif

#define restore_fpu(tsk) \
	do { restore_fpenv_lxrt((tsk)); (tsk)->flags |= PF_USEDFPU; } while (0)

static volatile int to_linux_depth[NR_RT_CPUS];

#define LOCK_LINUX(cpuid) \
do { \
	if (!to_linux_depth[cpuid]) { \
		set_bit(cpuid, &lxrt_hrt_flags); \
		rt_switch_to_real_time(cpuid); \
	} \
	to_linux_depth[cpuid]++; \
} while (0)

#define UNLOCK_LINUX(cpuid) \
do { \
	if (to_linux_depth[cpuid]) { \
		if (!--to_linux_depth[cpuid]) { \
			rt_switch_to_linux(cpuid); \
			clear_bit(cpuid, &lxrt_hrt_flags); \
		} \
	} else { \
		rt_printk("*** ERROR: EXCESS LINUX_UNLOCK ***\n"); \
	} \
} while (0)

#define ANTICIPATE

#define EXECTIME
#ifdef EXECTIME
static RTIME switch_time[NR_RT_CPUS];
#define KEXECTIME() \
do { \
	RTIME now; \
	now = rdtsc(); \
	if (!rt_current->lnxtsk) { \
		rt_current->exectime[0] += (now - switch_time[cpuid]); \
	} \
	switch_time[cpuid] = now; \
} while (0)

#define UEXECTIME() \
do { \
	RTIME now; \
	now = rdtsc(); \
	if (rt_current->is_hard) { \
		rt_current->exectime[0] += (now - switch_time[cpuid]); \
	} \
	switch_time[cpuid] = now; \
} while (0)
#else
#define KEXECTIME()
#define UEXECTIME()
#endif

static inline void make_current_soft(RT_TASK *rt_current)
{
        void rt_schedule(void);
        rt_current->state &= ~READY;
        rt_current->force_soft = 0;
        wake_up_srq.task[wake_up_srq.in] = rt_current->lnxtsk;
        wake_up_srq.in = (wake_up_srq.in + 1) & (MAX_SRQ - 1);
        rt_pend_linux_srq(wake_up_srq.srq);
        (rt_current->rprev)->rnext = rt_current->rnext;
        (rt_current->rnext)->rprev = rt_current->rprev;
        rt_schedule();
        rt_current->is_hard = 0;
        if ((rt_current->state |= READY) != READY) {
        	current->state = TASK_HARDREALTIME;
		rt_schedule();
        }
}

void rt_schedule(void)
{
	DECLARE_RT_CURRENT;
	RTIME intr_time, now;
	RT_TASK *task, *new_task;
	int prio, delay, preempt;

	prio = RT_LINUX_PRIORITY;
	ASSIGN_RT_CURRENT;
	sched_rqsted[cpuid] = 1;
repeat:
	task = new_task = &rt_linux_task;
	sched_get_global_lock(cpuid);
	RR_YIELD();
	if (oneshot_running) {
#ifdef ANTICIPATE
		rt_time_h = rdtsc() + rt_half_tick;
		wake_up_timed_tasks(cpuid);
#endif
		TASK_TO_SCHEDULE();
		RR_SETYT();

		intr_time = shot_fired ? rt_times.intr_time : rt_times.intr_time + ONESHOT_SPAN;
		RR_SPREMP();
		task = &rt_linux_task;
		while ((task = task->tnext) != &rt_linux_task) {
			if (task->priority <= prio && task->resume_time < intr_time) {
				rt_times.intr_time = task->resume_time;
				goto fire;
			}
		}
		if (preempt || (!shot_fired && prio == RT_LINUX_PRIORITY)) {
			if (preempt) {
				rt_times.intr_time = intr_time;
			}
fire:			shot_fired = 1;
			delay = (int)(rt_times.intr_time - (now = rdtsc())) - tuned.latency;
			if (delay >= tuned.setup_time_TIMER_CPUNIT) {
				delay = imuldiv(delay, TIMER_FREQ, tuned.cpu_freq);
			} else {
				delay = tuned.setup_time_TIMER_UNIT;
				rt_times.intr_time = now + (tuned.setup_time_TIMER_CPUNIT);
			}
			set_timer_chip(delay);
		}
	} else {
		TASK_TO_SCHEDULE();
		RR_SETYT();
	}
	sched_release_global_lock(cpuid);

	if (new_task != rt_current) {
		if (!new_task->lnxtsk || !rt_current->lnxtsk) {
			if (rt_current->lnxtsk) {
				LOCK_LINUX(cpuid);
				save_cr0_and_clts(linux_cr0);
				rt_linux_task.nextp = (void *)rt_current;
			} else if (new_task->lnxtsk) {
				rt_linux_task.prevp = (void *)new_task;
				new_task = (void *)rt_linux_task.nextp;
			}
			KEXECTIME();
			rt_exchange_tasks(rt_smp_current[cpuid], new_task);
			if (rt_current->lnxtsk) {
				UNLOCK_LINUX(cpuid);
				restore_cr0(linux_cr0);
				if (rt_current != (void *)rt_linux_task.prevp) {
					new_task = (void *)rt_linux_task.prevp;
					goto schedlnxtsk;
				}
			} else if (rt_current->uses_fpu) {
				enable_fpu();
				if (rt_current != fpu_task) {
					save_fpenv(fpu_task->fpu_reg);
					fpu_task = rt_current;
					restore_fpenv(fpu_task->fpu_reg);
				}
			}
			if (rt_current->signal) {
				(*rt_current->signal)();
			}
			hard_cli();
			return;
		}
schedlnxtsk:
		if (new_task->is_hard || rt_current->is_hard) {
			struct task_struct *prev;
#ifdef CONFIG_RTAI_ADEOS
			prev = arti_get_current(cpuid);
#else /* !CONFIG_RTAI_ADEOS */
			prev = current;
#endif /* CONFIG_RTAI_ADEOS */
			if (!rt_current->is_hard) {
				LOCK_LINUX(cpuid);
				rt_linux_task.lnxtsk = prev;
			}
			rt_smp_current[cpuid] = new_task;
			UEXECTIME();
			TASK_SWITCH(prev, new_task->lnxtsk, cpuid);
			if (prev->used_math) {
				restore_fpu(prev);
			}
	                if (rt_current->signal) {
                        	rt_current->signal();
                	}
			if (!rt_current->is_hard) {
				UNLOCK_LINUX(cpuid);
				if (rt_current->state != READY) {
					goto repeat;
				}
			} else if (rt_current->force_soft) {
				make_current_soft(rt_current);
			}
		} else {
			if (new_task != &rt_linux_task) {
				(new_task->lnxtsk)->rt_priority = BASE_SOFT_PRIORITY;
			}
			rt_smp_current[cpuid] = new_task;
			UNLOCK_LINUX(cpuid);
			rt_global_sti();
			schedule();
			rt_global_cli();
			if (rt_current != &rt_linux_task) {
				while (((rt_current)->state |= READY) != READY) {
					current->state = TASK_HARDREALTIME;
					rt_global_sti();
					schedule();
					rt_global_cli();
				}
				LOCK_LINUX(cpuid);
				(rt_current->lnxtsk)->rt_priority = BASE_SOFT_PRIORITY;
				enq_ready_task(rt_current);
				rt_smp_current[cpuid] = rt_current;
			}
        	}
        }
	hard_cli();
	return;
}


int rt_get_prio(RT_TASK *task)
{
	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	return task->base_priority;
}


int rt_get_inher_prio(RT_TASK *task)
{
	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	return task->base_priority;
}


void rt_spv_RMS(int cpuid)
{
	RT_TASK *task;
	int prio;
	if (cpuid < 0 || cpuid >= smp_num_cpus) {
		cpuid = hard_cpu_id();
	}
	prio = 0;
	task = &rt_linux_task;
	while ((task = task->next)) {
		RT_TASK *task, *htask;
		RTIME period;
		htask = 0;
		task = &rt_linux_task;
		period = RT_TIME_END;
		while ((task = task->next)) {
			if (task->priority >= 0 && task->policy >= 0 && task->period && task->period < period) {
				period = (htask = task)->period;
			}
		}
		if (htask) {
			htask->priority = -1;
			htask->base_priority = prio++;
		} else {
			goto ret;
		}
	}
ret:	task = &rt_linux_task;
	while ((task = task->next)) {
		if (task->priority < 0) {
			task->priority = task->base_priority;
		}
	}
	return;
}


int rt_change_prio(RT_TASK *task, int priority)
{
	unsigned long flags, schedmap;
	int prio;

	if (task->magic != RT_TASK_MAGIC || priority < 0) {
		return -EINVAL;
	}

	if ((prio = task->base_priority) == priority) {
		return prio;
	}
	flags = rt_global_save_flags_and_cli();
	if ((task->base_priority = priority) < task->priority || prio == task->priority) {
		QUEUE *q;
		do {
			task->priority = priority;
			if (task->state == READY) {
				(task->rprev)->rnext = task->rnext;
				(task->rnext)->rprev = task->rprev;
				enq_ready_task(task);
				set_bit(task->runnable_on_cpus, &schedmap);
			} else if ((q = task->blocked_on) && !((task->state & SEMAPHORE) && ((SEM *)q)->qtype)) {
				(task->queue.prev)->next = task->queue.next;
				(task->queue.next)->prev = task->queue.prev;
				while ((q = q->next) != task->blocked_on && (q->task)->priority <= priority);
				q->prev = (task->queue.prev = q->prev)->next  = &(task->queue);
				task->queue.next = q;
				set_bit(task->runnable_on_cpus, &schedmap);
			}
		} while ((task = task->prio_passed_to) && task->priority > priority);
		if (schedmap) {
			if (test_and_clear_bit(hard_cpu_id(), &schedmap)) {
				RT_SCHEDULE_MAP_BOTH(schedmap);
			} else {
				RT_SCHEDULE_MAP(schedmap);
			}
		}
	}
	rt_global_restore_flags(flags);
	return prio;
}


void rt_sched_lock(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (rt_current->priority >= 0) {
		rt_current->sched_lock_priority = rt_current->priority;
		sched_rqsted[cpuid] = rt_current->priority = -1;
	} else {
		rt_current->priority--;
	}
	rt_global_restore_flags(flags);
}


void rt_sched_unlock(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (rt_current->priority < 0 && !(++rt_current->priority)) {
		if ((rt_current->priority = rt_current->sched_lock_priority) != RT_LINUX_PRIORITY) {
			(rt_current->rprev)->rnext = rt_current->rnext;
			(rt_current->rnext)->rprev = rt_current->rprev;
			enq_ready_task(rt_current);
		}
		if (sched_rqsted[cpuid] > 0) {
			rt_schedule();
		}
	}
	rt_global_restore_flags(flags);
}


int clr_rtext(RT_TASK *task)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	QUEUE *q;

	if (task->magic != RT_TASK_MAGIC || task->priority == RT_LINUX_PRIORITY) {
		return -EINVAL;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (!(task->owndres & SEMHLF) || task == rt_current || rt_current->priority == RT_LINUX_PRIORITY) {
		call_exit_handlers(task);
		rem_timed_task(task);
		if (task->blocked_on) {
			(task->queue.prev)->next = task->queue.next;
			(task->queue.next)->prev = task->queue.prev;
			if (task->state & SEMAPHORE) {
				((SEM *)(task->blocked_on))->count++;
				if (((SEM *)(task->blocked_on))->type && ((SEM *)(task->blocked_on))->count > 1) {
					((SEM *)(task->blocked_on))->count = 1;
				}
			}
		}
		q = &(task->msg_queue);
		while ((q = q->next) != &(task->msg_queue)) {
			rem_timed_task(q->task);
			if ((q->task)->state != READY && ((q->task)->state &= ~(SEND | RPC | DELAYED)) == READY) {
				enq_ready_task(q->task);
			}       
			(q->task)->blocked_on = 0;
		}       
                q = &(task->ret_queue);
                while ((q = q->next) != &(task->ret_queue)) {
			rem_timed_task(q->task);
                       	if ((q->task)->state != READY && ((q->task)->state &= ~(RETURN | DELAYED)) == READY) {
				enq_ready_task(q->task);
			}       
			(q->task)->blocked_on = 0;
               	}
		if (!((task->prev)->next = task->next)) {
			rt_smp_linux_task[task->runnable_on_cpus].prev = task->prev;
		} else {
			(task->next)->prev = task->prev;
		}
		if (rt_smp_fpu_task[task->runnable_on_cpus] == task) {
			rt_smp_fpu_task[task->runnable_on_cpus] = rt_smp_linux_task + task->runnable_on_cpus;;
		}
		if (!task->lnxtsk) {
			frstk_srq.mp[frstk_srq.in] = task->stack_bottom;
			frstk_srq.in = (frstk_srq.in + 1) & (MAX_SRQ - 1);
			rt_pend_linux_srq(frstk_srq.srq);
		}
		task->magic = 0;
		rem_ready_task(task);
		task->state = 0;
		atomic_dec((atomic_t *)(tasks_per_cpu + task->runnable_on_cpus));
		if (task == rt_current) {
			rt_schedule();
		}
	} else {
		task->suspdepth = -0x7FFFFFFF;
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_task_delete(RT_TASK *task)
{
	if (!clr_rtext(task)) {
		if (task->lnxtsk) {
			start_stop_kthread(task, 0, 0, 0, 0, 0, 0);
		}
	}
	return 0;
}



int rt_get_task_state(RT_TASK *task)
{
	return task->state;
}


int rt_get_timer_cpu(void)
{
	return 1;
}


static void rt_timer_handler(void)
{
	DECLARE_RT_CURRENT;
	RTIME now;
	RT_TASK *task, *new_task;
	int prio, delay, preempt; 

	ASSIGN_RT_CURRENT;
	sched_rqsted[cpuid] = 1;
	task = new_task = &rt_linux_task;
	prio = RT_LINUX_PRIORITY;

#ifdef CONFIG_X86_REMOTE_DEBUG
	if (oneshot_timer) {    // Resync after possibly hitting a breakpoint
		rt_times.intr_time = rdtsc();
	}
#endif
	rt_times.tick_time = rt_times.intr_time;
	rt_time_h = rt_times.tick_time + rt_half_tick;
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		update_linux_timer();
	}

	sched_get_global_lock(cpuid);
	wake_up_timed_tasks(cpuid);
	RR_YIELD();
	TASK_TO_SCHEDULE();
	RR_SETYT();

	if (oneshot_timer) {
		rt_times.intr_time = rt_times.tick_time + ONESHOT_SPAN;
		RR_TPREMP();

		task = &rt_linux_task;
		while ((task = task->tnext) != &rt_linux_task) {
			if (task->priority <= prio && task->resume_time < rt_times.intr_time) {
				rt_times.intr_time = task->resume_time;
				shot_fired = 1;
				goto fire;
			}
		}
		if ((shot_fired = preempt)) {
			rt_times.intr_time = rt_times.linux_time > rt_times.tick_time ? rt_times.linux_time : rt_times.tick_time + (rt_times.linux_tick >> 1);
fire:			delay = (int)(rt_times.intr_time - (now = rdtsc())) - tuned.latency;
			if (delay >= tuned.setup_time_TIMER_CPUNIT) {
				delay = imuldiv(delay, TIMER_FREQ, tuned.cpu_freq);
			} else {
				delay = tuned.setup_time_TIMER_UNIT;
				rt_times.intr_time = now + (tuned.setup_time_TIMER_CPUNIT);
			}
			set_timer_chip(delay);
		}
	} else {
		rt_times.intr_time += rt_times.periodic_tick;
                rt_set_timer_delay(0);
	}
	sched_release_global_lock(cpuid);

	if (new_task != rt_current) {
		if (!new_task->lnxtsk || !rt_current->lnxtsk) {
			if (rt_current->lnxtsk) {
				LOCK_LINUX(cpuid);
				save_cr0_and_clts(linux_cr0);
				rt_linux_task.nextp = (void *)rt_current;
			} else if (new_task->lnxtsk) {
				rt_linux_task.prevp = (void *)new_task;
				new_task = (void *)rt_linux_task.nextp;
			}
			KEXECTIME();
			rt_exchange_tasks(rt_smp_current[cpuid], new_task);
			if (rt_current->lnxtsk) {
				UNLOCK_LINUX(cpuid);
				restore_cr0(linux_cr0);
				if (rt_current != (void *)rt_linux_task.prevp) {
					new_task = (void *)rt_linux_task.prevp;
					goto schedlnxtsk;
				}
			} else if (rt_current->uses_fpu) {
				enable_fpu();
				if (rt_current != fpu_task) {
					save_fpenv(fpu_task->fpu_reg);
					fpu_task = rt_current;
					restore_fpenv(fpu_task->fpu_reg);
				}
			}
			if (rt_current->signal) {
				(*rt_current->signal)();
			}	
			hard_cli();
			return;
		}
schedlnxtsk:
		if (new_task->is_hard || rt_current->is_hard) {
			struct task_struct *prev;
#ifdef CONFIG_RTAI_ADEOS
			prev = arti_get_current(cpuid);
#else /* !CONFIG_RTAI_ADEOS */
			prev = current;
#endif /* CONFIG_RTAI_ADEOS */
			if (!rt_current->is_hard) {
				LOCK_LINUX(cpuid);
				rt_linux_task.lnxtsk = prev;
			}
			rt_smp_current[cpuid] = new_task;
			UEXECTIME();
			TASK_SWITCH(prev, new_task->lnxtsk, cpuid);
			if (prev->used_math) {
				restore_fpu(prev);
			}
			if (rt_current->signal) {
        	       	        rt_current->signal();
                	}
			if (!rt_current->is_hard) {
				UNLOCK_LINUX(cpuid);
				if (rt_current->state != READY) {
					rt_printk("***** COULD THIS HAPPEN? ****\n");
				}
			} else if (rt_current->force_soft) {
				make_current_soft(rt_current);
			}
		} else {
			rt_printk("***** THIS SHOULD NOT HAPPEN ****\n");
		}
        }
	hard_cli();
	return;
}


static void recover_jiffies(int irq, void *dev_id, struct pt_regs *regs)
{
	rt_global_cli();
	if (linux_times->tick_time >= linux_times->linux_time) {
		linux_times->linux_time += linux_times->linux_tick;
		rt_pend_linux_irq(TIMER_8254_IRQ);
	}
	rt_global_sti();
	BROADCAST_TO_LOCAL_TIMERS();
} 


int rt_is_hard_timer_running(void) 
{ 
	int cpuid, running;
	for (running = cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		if (rt_time_h) {
			set_bit(cpuid, &running);
		}
	}
	return running;
}


void rt_set_periodic_mode(void) 
{ 
	int cpuid;
	stop_rt_timer();
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		oneshot_timer = oneshot_running = 0;
	}
}


void rt_set_oneshot_mode(void)
{ 
	int cpuid;
	stop_rt_timer();
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		oneshot_timer = 1;
	}
}


#ifdef CONFIG_SMP

void start_rt_apic_timers(struct apic_timer_setup_data *setup_data, unsigned int rcvr_jiffies_cpuid)
{
	unsigned long flags, cpuid;

	rt_request_apic_timers(rt_timer_handler, setup_data);
	flags = rt_global_save_flags_and_cli();
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		if (setup_data[cpuid].mode > 0) {
			oneshot_timer = oneshot_running = 0;
			tuned.timers_tol[cpuid] = rt_half_tick = (rt_times.periodic_tick + 1)>>1;
		} else {
			oneshot_timer = oneshot_running = 1;
			tuned.timers_tol[cpuid] = rt_half_tick = (tuned.latency + 1)>>1;
		}
		rt_time_h = rt_times.tick_time + rt_half_tick;
		shot_fired = 1;
	}
	linux_times = rt_smp_times + (rcvr_jiffies_cpuid < NR_RT_CPUS ? rcvr_jiffies_cpuid : 0);
	rt_global_restore_flags(flags);
	rt_free_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers);
	rt_request_linux_irq(TIMER_8254_IRQ, recover_jiffies, "rtai_jif_chk", recover_jiffies);
}


RTIME start_rt_timer(int period)
{
	int cpuid;
	struct apic_timer_setup_data setup_data[NR_RT_CPUS];
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		setup_data[cpuid].mode = oneshot_timer ? 0 : 1;
		setup_data[cpuid].count = count2nano(period);
	}
	start_rt_apic_timers(setup_data, hard_cpu_id());
	return period;
}


RTIME start_rt_timer_cpuid(int period, int cpuid)
{
	return start_rt_timer(period);
}


void stop_rt_timer(void)
{
	unsigned long flags;
	int cpuid;
	rt_free_linux_irq(TIMER_8254_IRQ, recover_jiffies);
	rt_free_apic_timers();
	flags = rt_global_save_flags_and_cli();
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		oneshot_timer = oneshot_running = 0;
	}
	rt_global_restore_flags(flags);
	rt_busy_sleep(10000000);
}

#else


RTIME start_rt_timer(int period)
{
#define cpuid 0
#undef rt_times

        unsigned long flags;
        flags = rt_global_save_flags_and_cli();
        if (oneshot_timer) {
                rt_request_timer(rt_timer_handler, 0, 0);
                tuned.timers_tol[0] = rt_half_tick = (tuned.latency + 1)>>1;
                oneshot_running = shot_fired = 1;
        } else {
                rt_request_timer(rt_timer_handler, period > LATCH? LATCH: period, 0);
                tuned.timers_tol[0] = rt_half_tick = (rt_times.periodic_tick + 1)>>1;
        }
	rt_smp_times[cpuid].linux_tick    = rt_times.linux_tick;
	rt_smp_times[cpuid].tick_time     = rt_times.tick_time;
	rt_smp_times[cpuid].intr_time     = rt_times.intr_time;
	rt_smp_times[cpuid].linux_time    = rt_times.linux_time;
	rt_smp_times[cpuid].periodic_tick = rt_times.periodic_tick;
        rt_time_h = rt_times.tick_time + rt_half_tick;
	linux_times = rt_smp_times;
        rt_global_restore_flags(flags);
        rt_request_linux_irq(TIMER_8254_IRQ, recover_jiffies, "rtai_jif_chk", recover_jiffies);
        return period;

#undef cpuid
#define rt_times (rt_smp_times[cpuid])
}


void start_rt_apic_timers(struct apic_timer_setup_data *setup_mode, unsigned int rcvr_jiffies_cpuid)
{
	int cpuid, period;

	period = 0;
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		period += setup_mode[cpuid].mode;
	}
	if (period == NR_RT_CPUS) {
		period = 2000000000;
		for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
			if (setup_mode[cpuid].count < period) {
				period = setup_mode[cpuid].count;
			}
		}
		start_rt_timer(nano2count(period));
	} else {
		rt_set_oneshot_mode();
		start_rt_timer(0);
	}
}


void stop_rt_timer(void)
{
        unsigned long flags;
        rt_free_linux_irq(TIMER_8254_IRQ, recover_jiffies);
        rt_free_timer();
        flags = rt_global_save_flags_and_cli();
	rt_smp_oneshot_timer[0] = rt_smp_oneshot_running[0] = 0;
        rt_global_restore_flags(flags);
        rt_busy_sleep(10000000);
}

#endif


RT_TASK *rt_whoami(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	rt_global_restore_flags(flags);
	return rt_current;
}


int rt_sched_type(void)
{
	return MUP_SCHED;
}


int rt_task_signal_handler(RT_TASK *task, void (*handler)(void))
{
	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	task->signal = handler;
	return 0;
}


int rt_task_use_fpu(RT_TASK *task, int use_fpu_flag)
{
	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	task->uses_fpu = use_fpu_flag ? 1 : 0;
	return 0;
}


void rt_linux_use_fpu(int use_fpu_flag)
{
	int cpuid;
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		rt_linux_task.uses_fpu = use_fpu_flag ? 1 : 0;
	}
}


void rt_preempt_always(int yes_no)
{
	int cpuid;
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		rt_smp_preempt_always[cpuid] = yes_no ? 1 : 0;
	}
}


void rt_preempt_always_cpuid(int yes_no, unsigned int cpuid)
{
	rt_smp_preempt_always[cpuid] = yes_no ? 1 : 0;
}


RT_TRAP_HANDLER rt_set_task_trap_handler( RT_TASK *task, unsigned int vec, RT_TRAP_HANDLER handler)
{
	RT_TRAP_HANDLER old_handler;

	if (!task || (vec >= RTAI_NR_TRAPS)) {
		return (RT_TRAP_HANDLER) -EINVAL;
	}
	old_handler = task->task_trap_handler[vec];
	task->task_trap_handler[vec] = handler;
	return old_handler;
}


#ifndef CONFIG_RTAI_DYN_MM_MODULE
extern unsigned int granularity;
MODULE_PARM(granularity, "i");

extern int low_chk_ref;
MODULE_PARM(low_chk_ref, "i");

extern int low_data_mark;
MODULE_PARM(low_data_mark, "i");
#endif

static int OneShot = ONE_SHOT;
MODULE_PARM(OneShot, "i");

static int PreemptAlways = PREEMPT_ALWAYS;
MODULE_PARM(PreemptAlways, "i");

static int Latency = TIMER_LATENCY;
MODULE_PARM(Latency, "i");

static int SetupTimeTIMER = TIMER_SETUP_TIME;
MODULE_PARM(SetupTimeTIMER, "i");

static void usp_cleanup_module(void);
static int usp_init_module(void);

static void frstk_srq_handler(void)
{
        while (frstk_srq.out != frstk_srq.in) {
		sched_free(frstk_srq.mp[frstk_srq.out]);
		frstk_srq.out = (frstk_srq.out + 1) & (MAX_SRQ - 1);
	}
}

int init_module(void)
{
	int cpuid;
	sched_mem_init();
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		rt_linux_task.uses_fpu = 1;
		rt_linux_task.magic = 0;
		rt_linux_task.policy = rt_linux_task.is_hard = 0;
		rt_linux_task.runnable_on_cpus = cpuid;
		rt_linux_task.state = READY;
		rt_linux_task.msg_queue.prev = &(rt_linux_task.msg_queue);      
		rt_linux_task.msg_queue.next = &(rt_linux_task.msg_queue);      
		rt_linux_task.msg_queue.task = &rt_linux_task;    
		rt_linux_task.msg = 0;  
		rt_linux_task.ret_queue.prev = &(rt_linux_task.ret_queue);
		rt_linux_task.ret_queue.next = &(rt_linux_task.ret_queue);
		rt_linux_task.ret_queue.task = NOTHING;
		rt_linux_task.priority = RT_LINUX_PRIORITY;
		rt_linux_task.base_priority = RT_LINUX_PRIORITY;
		rt_linux_task.signal = 0;
		rt_linux_task.prev = &rt_linux_task;
                rt_linux_task.resume_time = RT_TIME_END;
                rt_linux_task.tprev = rt_linux_task.tnext =
                rt_linux_task.rprev = rt_linux_task.rnext = &rt_linux_task;
		rt_linux_task.next = 0;
		rt_linux_task.lnxtsk = current;
		rt_smp_current[cpuid] = &rt_linux_task;
		rt_smp_fpu_task[cpuid] = &rt_linux_task;
		oneshot_timer = OneShot ? 1 : 0;
		oneshot_running = 0;
		preempt_always = PreemptAlways ? 1 : 0;
	}
	tuned.latency = imuldiv(Latency, tuned.cpu_freq, 1000000000);
	tuned.setup_time_TIMER_CPUNIT = imuldiv( SetupTimeTIMER, 
						 tuned.cpu_freq, 
						 1000000000);
	tuned.setup_time_TIMER_UNIT   = imuldiv( SetupTimeTIMER, 
						 TIMER_FREQ, 
						 1000000000);
	tuned.timers_tol[0] = 0;
#ifdef CONFIG_PROC_FS
	rtai_proc_sched_register();
#endif
	printk("\n***** STARTING THE NEWLXRT-NOBUDDY REAL TIME SCHEDULER *****");
	printk("\n***<> LINUX TICK AT %d (HZ) <>***", HZ);
	printk("\n***<> CALIBRATED CPU FREQUENCY %lu (HZ) <>***", tuned.cpu_freq);
#ifdef CONFIG_SMP
	printk("\n***<> CALIBRATED APIC_INTERRUPT-TO-SCHEDULER LATENCY %d (ns) <>***", imuldiv(tuned.latency - tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
	printk("\n***<> CALIBRATED ONE SHOT APIC SETUP TIME %d (ns) <>***\n\n", imuldiv(tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
#else
	printk("\n***<> CALIBRATED 8254-TIMER-INTERRUPT-TO-SCHEDULER LATENCY %d (ns) <>***", imuldiv(tuned.latency - tuned.setup_time_TIMER_CPUNIT, 1000000000,
tuned.cpu_freq));
	printk("\n***<> CALIBRATED ONE SHOT SETUP TIME %d (ns) <>***\n\n", imuldiv(tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
#endif
	rt_mount_rtai();
// 0x7dd763ad == nam2num("MEMSRQ").
	if ((frstk_srq.srq = rt_request_srq(0x7dd763ad, frstk_srq_handler, 0)) < 0) {
		printk("MEM SRQ: no sysrq available.\n");
		return frstk_srq.srq;
	}
	frstk_srq.in = frstk_srq.out = 0;
	rt_request_sched_ipi();
	usp_init_module();
	return 0;
}


void cleanup_module(void)
{
	int cpuid;
	stop_rt_timer();
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		while (rt_linux_task.next) {
			rt_task_delete(rt_linux_task.next);
		}
	}
	usp_cleanup_module();
#ifdef CONFIG_PROC_FS
        rtai_proc_sched_unregister();
#endif
	while (frstk_srq.out != frstk_srq.in);
	if (rt_free_srq(frstk_srq.srq) < 0) {
		printk("MEM SRQ: frstk_srq %d illegal or already free.\n", frstk_srq.srq);
	}
	rt_free_sched_ipi();
	sched_mem_end();
	rt_umount_rtai();
	printk("\n***** THE NEWLXRT-NOBUDDY REAL TIME SCHEDULER HAS BEEN REMOVED *****\n\n");
}

#ifdef CONFIG_PROC_FS
/* ----------------------< proc filesystem section >----------------------*/

static int rtai_read_sched(char *page, char **start, off_t off, int count,
                           int *eof, void *data)
{
	PROC_PRINT_VARS;
        int cpuid, i = 1;
	unsigned long t;
	RT_TASK *task;

	PROC_PRINT("\nRTAI NEWLXRT Real Time Task Scheduler.\n\n");
	PROC_PRINT("    Calibrated CPU Frequency: %lu Hz\n", tuned.cpu_freq);
	PROC_PRINT("    Calibrated 8254 interrupt to scheduler latency: %d ns\n", imuldiv(tuned.latency - tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
	PROC_PRINT("    Calibrated one shot setup time: %d ns\n\n",
                  imuldiv(tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
	PROC_PRINT("Number of RT CPUs in system: %d\n\n", NR_RT_CPUS);

	PROC_PRINT("Priority  Period(ns)  FPU  Sig  State  CPU  Task  HD/SF  PID  RT_TASK *  TIME\n" );
	PROC_PRINT("------------------------------------------------------------------------------\n" );
        for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
                task = &rt_linux_task;
/*
* Display all the active RT tasks and their state.
*
* Note: As a temporary hack the tasks are given an id which is
*       the order they appear in the task list, needs fixing!
*/
		while ((task = task->next)) {
/*
* The display for the task period is set to an integer (%d) as 64 bit
* numbers are not currently handled correctly by the kernel routines.
* Hence the period display will be wrong for time periods > ~4 secs.
*/
			t = 0;
			if ((!task->lnxtsk || task->is_hard) && task->exectime[1]) {
				t = 1000UL*(unsigned long)llimd(task->exectime[0], 10, tuned.cpu_freq)/(unsigned long)llimd(rdtsc() - task->exectime[1], 10, tuned.cpu_freq);
			}
			PROC_PRINT("%-10d %-11lu %-4s %-4s 0x%-4x %-4d %-4d   %-4d %-4d  %p   %-lu\n",
                               task->priority,
                               (unsigned long)count2nano_cpuid(task->period, task->runnable_on_cpus),
                               task->uses_fpu ? "Yes" : "No",
                               task->signal ? "Yes" : "No",
                               task->state,
                               cpuid,
                               i,
			       task->is_hard,
			       task->lnxtsk ? task->lnxtsk->pid : 0,
			       task, t);
			i++;
                } /* End while loop - display all RT tasks on a CPU. */

		PROC_PRINT("TIMED\n");
		task = &rt_linux_task;
		while ((task = task->tnext) != &rt_linux_task) {
			PROC_PRINT("> %p ", task);
		}
		PROC_PRINT("\nREADY\n");
		task = &rt_linux_task;
		while ((task = task->rnext) != &rt_linux_task) {
			PROC_PRINT("> %p ", task);
		}

        }  /* End for loop - display RT tasks on all CPUs. */

	PROC_PRINT_DONE;

}  /* End function - rtai_read_sched */


static int rtai_proc_sched_register(void) 
{
        struct proc_dir_entry *proc_sched_ent;


        proc_sched_ent = create_proc_entry("scheduler", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
        if (!proc_sched_ent) {
                printk("Unable to initialize /proc/rtai/scheduler\n");
                return(-1);
        }
        proc_sched_ent->read_proc = rtai_read_sched;
        return(0);
}  /* End function - rtai_proc_sched_register */


static void rtai_proc_sched_unregister(void) 
{
        remove_proc_entry("scheduler", rtai_proc_root);
}  /* End function - rtai_proc_sched_unregister */

/* ------------------< end of proc filesystem section >------------------*/
#endif /* CONFIG_PROC_FS */


/* ++++++++++++++++++++++++++ TIME CONVERSIONS +++++++++++++++++++++++++++++ */

RTIME count2nano(RTIME counts)
{
	int sign;

	if (counts > 0) {
		sign = 1;
	} else {
		sign = 0;
		counts = - counts;
	}
	counts = oneshot_timer_cpuid ?
		 llimd(counts, 1000000000, tuned.cpu_freq):
		 llimd(counts, 1000000000, TIMER_FREQ);
	return sign ? counts : - counts;
}


RTIME nano2count(RTIME ns)
{
	int sign;

	if (ns > 0) {
		sign = 1;
	} else {
		sign = 0;
		ns = - ns;
	}
	ns =  oneshot_timer_cpuid ?
	      llimd(ns, tuned.cpu_freq, 1000000000) :
	      llimd(ns, TIMER_FREQ, 1000000000);
	return sign ? ns : - ns;
}

RTIME count2nano_cpuid(RTIME counts, unsigned int cpuid)
{
	int sign;

	if (counts > 0) {
		sign = 1;
	} else {
		sign = 0;
		counts = - counts;
	}
	counts = oneshot_timer ?
		 llimd(counts, 1000000000, tuned.cpu_freq):
		 llimd(counts, 1000000000, TIMER_FREQ);
	return sign ? counts : - counts;
}


RTIME nano2count_cpuid(RTIME ns, unsigned int cpuid)
{
	int sign;

	if (ns > 0) {
		sign = 1;
	} else {
		sign = 0;
		ns = - ns;
	}
	ns =  oneshot_timer ?
	      llimd(ns, tuned.cpu_freq, 1000000000) :
	      llimd(ns, TIMER_FREQ, 1000000000);
	return sign ? ns : - ns;
}

/* +++++++++++++++++++++++++++++++ TIMINGS ++++++++++++++++++++++++++++++++++ */

void rt_gettimeorig(RTIME time_orig[])
{
	unsigned long flags;
	struct timeval tv;
	hard_save_flags_and_cli(flags);
	do_gettimeofday(&tv);
	time_orig[0] = rdtsc();
	hard_restore_flags(flags);
	time_orig[0] = tv.tv_sec*(long long)tuned.cpu_freq + llimd(tv.tv_usec, tuned.cpu_freq, 1000000) - time_orig[0];
	time_orig[1] = llimd(time_orig[0], 1000000000, tuned.cpu_freq);
}

static inline RTIME get_time(void)
{
	int cpuid = hard_cpu_id();
	return oneshot_timer ? rdtsc(): rt_times.tick_time;
}


RTIME rt_get_time(void)
{
	return get_time();
}


RTIME rt_get_time_cpuid(unsigned int cpuid)
{
	return oneshot_timer ? rdtsc(): rt_times.tick_time;
}


RTIME rt_get_time_ns(void)
{
	int cpuid = hard_cpu_id();
	return oneshot_timer ? llimd(rdtsc(), 1000000000, tuned.cpu_freq) :
	    		       llimd(rt_times.tick_time, 1000000000, TIMER_FREQ);
}


RTIME rt_get_time_ns_cpuid(unsigned int cpuid)
{
	return oneshot_timer ? llimd(rdtsc(), 1000000000, tuned.cpu_freq) :
			       llimd(rt_times.tick_time, 1000000000, TIMER_FREQ);
}


RTIME rt_get_cpu_time_ns(void)
{
	return llimd(rdtsc(), 1000000000, tuned.cpu_freq);
}


void rt_task_yield(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	{
		RT_TASK *task;
		task = rt_current->rnext;
		while (rt_current->priority == task->priority) {
			task = task->rnext;
		}
		if (task != rt_current->rnext) {
			(rt_current->rprev)->rnext = rt_current->rnext;
			(rt_current->rnext)->rprev = rt_current->rprev;
			task->rprev = (rt_current->rprev = task->rprev)->rnext = rt_current;
			rt_current->rnext = task;
			rt_schedule();
		}
	}
	rt_global_restore_flags(flags);
}


int rt_task_suspend(RT_TASK *task)
{
	unsigned long flags;

	if (!task) {
		task = RT_CURRENT;
	} else if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}

	flags = rt_global_save_flags_and_cli();
	if (!task->suspdepth++ && !task->owndres) {
		rem_ready_task(task);
		task->state |= SUSPENDED;
		if (task == RT_CURRENT) {
			rt_schedule();
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_task_resume(RT_TASK *task)
{
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}

	flags = rt_global_save_flags_and_cli();
	if (!(--task->suspdepth)) {
		rem_timed_task(task);
		if (((task->state &= ~SUSPENDED) & ~DELAYED) == READY) {
			enq_ready_task(task);
			RT_SCHEDULE(task, hard_cpu_id());
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_task_make_periodic_relative_ns(RT_TASK *task, RTIME start_delay, RTIME period)
{
	long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	start_delay = nano2count_cpuid(start_delay, task->runnable_on_cpus);
	period = nano2count_cpuid(period, task->runnable_on_cpus);
	flags = rt_global_save_flags_and_cli();
	task->resume_time = rt_get_time_cpuid(task->runnable_on_cpus) + start_delay;
	task->period = period;
	task->suspdepth = 0;
        if (!(task->state & DELAYED)) {
		rem_ready_task(task);
		task->state = (task->state & ~SUSPENDED) | DELAYED;
		enq_timed_task(task);
}
	RT_SCHEDULE(task, hard_cpu_id());
	rt_global_restore_flags(flags);
	return 0;
}


int rt_task_make_periodic(RT_TASK *task, RTIME start_time, RTIME period)
{
	long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	flags = rt_global_save_flags_and_cli();
	task->resume_time = start_time;
	task->period = period;
	task->suspdepth = 0;
        if (!(task->state & DELAYED)) {
		rem_ready_task(task);
		task->state = (task->state & ~SUSPENDED) | DELAYED;
		enq_timed_task(task);
	}
	RT_SCHEDULE(task, hard_cpu_id());
	rt_global_restore_flags(flags);
	return 0;
}


void rt_task_wait_period(void)
{
	DECLARE_RT_CURRENT;
	long flags;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (rt_current->resync_frame) { // Request from watchdog
	    	rt_current->resync_frame = 0;
		rt_current->resume_time = rt_get_time();
	} else if ((rt_current->resume_time += rt_current->period) > rt_time_h) {
		rt_current->state |= DELAYED;
		rem_ready_current(rt_current);
		enq_timed_task(rt_current);
		rt_schedule();
	}
	rt_global_restore_flags(flags);
}


void rt_task_set_resume_end_times(RTIME resume, RTIME end)
{
	DECLARE_RT_CURRENT;
	long flags;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	rt_current->policy   = -1;
	rt_current->priority =  0;
	if (resume > 0) {
		rt_current->resume_time = resume;
	} else {
		rt_current->resume_time -= resume;
	}
	if (end > 0) {
		rt_current->period = end;
	} else {
		rt_current->period = rt_current->resume_time - end;
	}
	rt_current->state |= DELAYED;
	rem_ready_current(rt_current);
	enq_timed_task(rt_current);
	rt_schedule();
	rt_global_restore_flags(flags);
}


int rt_set_resume_time(RT_TASK *task, RTIME new_resume_time)
{
	long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}

	flags = rt_global_save_flags_and_cli();
	if (task->state & DELAYED) {
		task->resume_time = new_resume_time;
		if ((task->tnext != &rt_smp_linux_task[task->runnable_on_cpus] && (new_resume_time - (task->tnext)->resume_time) > 0) || (new_resume_time - (task->tprev)->resume_time) < 0) {
			rem_timed_task(task);
			enq_timed_task(task);
			RT_SCHEDULE(task, hard_cpu_id());
			rt_global_restore_flags(flags);
			return 0;
        	}
        }
	rt_global_restore_flags(flags);
	return -ETIME;
}


int rt_set_period(RT_TASK *task, RTIME new_period)
{
	long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	hard_save_flags_and_cli(flags);
	task->period = new_period;
	hard_restore_flags(flags);
	return 0;
}


RTIME next_period(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	rt_global_restore_flags(flags);
	return rt_current->resume_time + rt_current->period;
}


void rt_busy_sleep(int ns)
{
	RTIME end_time;
	end_time = rdtsc() + llimd(ns, tuned.cpu_freq, 1000000000);
	while (rdtsc() < end_time);
}


void rt_sleep(RTIME delay)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((rt_current->resume_time = (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay) > rt_time_h) {
		rt_current->state |= DELAYED;
		rem_ready_current(rt_current);
		enq_timed_task(rt_current);
		rt_schedule();
	}
	rt_global_restore_flags(flags);
}


void rt_sleep_until(RTIME time)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((rt_current->resume_time = time) > rt_time_h) {
		rt_current->state |= DELAYED;
		rem_ready_current(rt_current);
		enq_timed_task(rt_current);
		rt_schedule();
	}
	rt_global_restore_flags(flags);
}

int rt_task_wakeup_sleeping(RT_TASK *task)
{
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}

	flags = rt_global_save_flags_and_cli();
	rem_timed_task(task);
	if (task->state != READY && (task->state &= ~DELAYED) == READY) {
		enq_ready_task(task);
		RT_SCHEDULE(task, hard_cpu_id());
	}
	rt_global_restore_flags(flags);
	return 0;
}

/* +++++++++++++++++++++++++++++ SEMAPHORES ++++++++++++++++++++++++++++++++ */

static inline void enqueue_blocked(RT_TASK *task, QUEUE *queue, int qtype)
{
	QUEUE *q;
	task->blocked_on = (q = queue);
	if (!qtype) {
		while ((q = q->next) != queue && (q->task)->priority <= task->priority);
	}
	q->prev = (task->queue.prev = q->prev)->next  = &(task->queue);
	task->queue.next = q;
}


static inline void dequeue_blocked(RT_TASK *task)
{
	task->prio_passed_to     = NOTHING;
	(task->queue.prev)->next = task->queue.next;
	(task->queue.next)->prev = task->queue.prev;
	task->blocked_on    	 = NOTHING;
}


static __volatile__ inline unsigned long pass_prio(RT_TASK *to, RT_TASK *from)
{
	unsigned long schedmap;
	QUEUE *q;
	from->prio_passed_to = to;
	schedmap = 0;
	while (to && to->priority > from->priority) {
		to->priority = from->priority;
		if (to->state == READY) {
			(to->rprev)->rnext = to->rnext;
			(to->rnext)->rprev = to->rprev;
			enq_ready_task(to);
			set_bit(to->runnable_on_cpus, &schedmap);
		} else if ((q = to->blocked_on) && !((to->state & SEMAPHORE) &&  ((SEM *)q)->qtype)) {
			(to->queue.prev)->next = to->queue.next;
			(to->queue.next)->prev = to->queue.prev;
			while ((q = q->next) != to->blocked_on && (q->task)->priority <= to->priority);
			q->prev = (to->queue.prev = q->prev)->next  = &(to->queue);
			to->queue.next = q;
		}
		to = to->prio_passed_to;
	}
	return schedmap;
}


void rt_typed_sem_init(SEM *sem, int value, int type)
{
	sem->magic = RT_SEM_MAGIC;
	sem->count = value;
	sem->qtype = type != RES_SEM && (type & FIFO_Q) ? 1 : 0;
	type = (type & 3) - 2;
	if ((sem->type = type) < 0 && value > 1) {
		sem->count = 1;
	} else if (type > 0) {
		sem->type = sem->count = 1;
	}
	sem->queue.prev = &(sem->queue);
	sem->queue.next = &(sem->queue);
	sem->queue.task = sem->owndby = 0;
}


void rt_sem_init(SEM *sem, int value)
{
	rt_typed_sem_init(sem, value, CNT_SEM);
}


int rt_sem_delete(SEM *sem)
{
	unsigned long flags;
	RT_TASK *task;
	unsigned long schedmap;
	QUEUE *q;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	schedmap = 0;
	q = &(sem->queue);
	flags = rt_global_save_flags_and_cli();
	sem->magic = 0;
	while ((q = q->next) != &(sem->queue) && (task = q->task)) {
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			enq_ready_task(task);
			set_bit(task->runnable_on_cpus, &schedmap);
		}
	}
	clear_bit(hard_cpu_id(), &schedmap);
	if ((task = sem->owndby) && sem->type > 0) {
		int sched;
		if (task->owndres & SEMHLF) {
			--task->owndres;
		}
		if (!task->owndres) {
			sched = renq_ready_task(task, task->base_priority);
		} else if (!(task->owndres & SEMHLF)) {
			int priority;
                        sched = renq_ready_task(task, task->base_priority > (priority = ((task->msg_queue.next)->task)->priority) ? priority : task->base_priority);
		} else {
			sched = 0;
		}
		if (task->suspdepth) {
			if (task->suspdepth > 0) {
				task->state |= SUSPENDED;
				rem_ready_task(task);
				sched = 1;
			} else {
				rt_task_delete(task);
			}
		}
		if (sched) {
			if (schedmap) {
				RT_SCHEDULE_MAP_BOTH(schedmap);
			} else {
				rt_schedule();
			}
		}
	} else {
		RT_SCHEDULE_MAP(schedmap);
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_count(SEM *sem)
{
	return sem->count;
}


int rt_sem_signal(SEM *sem)
{
	unsigned long flags;
	RT_TASK *task;
	int tosched;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (sem->type) {
		if (sem->type > 1) {
			sem->type--;
			rt_global_restore_flags(flags);
			return 0;
		}
		if (++sem->count > 1) {
			sem->count = 1;
		}
	} else {
		sem->count++;
	}
	if ((task = (sem->queue.next)->task)) {
		dequeue_blocked(task);
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			enq_ready_task(task);
			if (sem->type <= 0) {
				RT_SCHEDULE(task, hard_cpu_id());
				rt_global_restore_flags(flags);
				return 0;
			}
			tosched = 1;
			goto res;
		}
	}
	tosched = 0;
res:	if (sem->type > 0) {
		DECLARE_RT_CURRENT;
		int sched;
		ASSIGN_RT_CURRENT;
		sem->owndby = 0;
		if (rt_current->owndres & SEMHLF) {
			--rt_current->owndres;
		}
		if (!rt_current->owndres) {
			sched = renq_current(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_current(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
		} else {
			sched = 0;
		}
		if (rt_current->suspdepth) {
			if (rt_current->suspdepth > 0) {
				rt_current->state |= SUSPENDED;
				rem_ready_current(rt_current);
                        	sched = 1;
			} else {
				rt_task_delete(rt_current);
			}
		}
		if (sched) {
			if (tosched) {
				RT_SCHEDULE_BOTH(task, cpuid);
			} else {
				rt_schedule();
			}
		} else if (tosched) {
			RT_SCHEDULE(task, cpuid);
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_broadcast(SEM *sem)
{
	unsigned long flags, schedmap;
	RT_TASK *task;
	QUEUE *q;
	DECLARE_RT_CURRENT;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	schedmap = 0;
	q = &(sem->queue);
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	while ((q = q->next) != &(sem->queue)) {
		dequeue_blocked(task = q->task);
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			enq_ready_task(task);
			set_bit(task->runnable_on_cpus, &schedmap);
		}
	}
	sem->count = 0;
	sem->queue.prev = sem->queue.next = &(sem->queue);
	if (schedmap) {
		if (test_and_clear_bit(hard_cpu_id(), &schedmap)) {
			RT_SCHEDULE_MAP_BOTH(schedmap);
		} else {
			RT_SCHEDULE_MAP(schedmap);
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_wait(SEM *sem)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	int count;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((count = sem->count) <= 0) {
		unsigned long schedmap;
		if (sem->type > 0) {
			if (sem->owndby == rt_current) {
				sem->type++;
				rt_global_restore_flags(flags);
				return count;
			}
			schedmap = pass_prio(sem->owndby, rt_current);
		} else {
			schedmap = 0;
		}
		sem->count--;
		rt_current->state |= SEMAPHORE;
		rem_ready_current(rt_current);
		enqueue_blocked(rt_current, &sem->queue, sem->qtype);
		RT_SCHEDULE_MAP_BOTH(schedmap);
		if (rt_current->blocked_on || sem->magic != RT_SEM_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return SEM_ERR;
		} else { 
			count = sem->count;
		}
	} else {
		sem->count--;
	}
	if (sem->type > 0) {
		(sem->owndby = rt_current)->owndres++;
	}
	rt_global_restore_flags(flags);
	return count;
}


int rt_sem_wait_if(SEM *sem)
{
	int count;
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if ((count = sem->count) <= 0) {
		if (sem->type > 0 && sem->owndby == RT_CURRENT) {
			sem->type++;
			rt_global_restore_flags(flags);
			return 0;
		}
	} else {
		sem->count--;
		if (sem->type > 0) {
			(sem->owndby = RT_CURRENT)->owndres++;
		}
	}
	rt_global_restore_flags(flags);
	return count;
}


#if 0
int rt_sem_wait_if(SEM *sem)
{
	int count;
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if ((count = sem->count) > 0) {
		if (sem->type > 0) {
			DECLARE_RT_CURRENT;
			ASSIGN_RT_CURRENT;
			if (sem->owndby == rt_current) {
				sem->type++;
				rt_global_restore_flags(flags);
				return 0;
			}
			(sem->owndby = rt_current)->owndres++;
		}
		sem->count--;
	}
	rt_global_restore_flags(flags);
	return count;
}
#endif


int rt_sem_wait_until(SEM *sem, RTIME time)
{
	DECLARE_RT_CURRENT;
	int count;
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((count = sem->count) <= 0) {
		rt_current->blocked_on = &sem->queue;
		if ((rt_current->resume_time = time) > rt_time_h) {
			unsigned long schedmap;
			if (sem->type > 0) {
				if (sem->owndby == rt_current) {
					sem->type++;
					rt_global_restore_flags(flags);
					return 0;
				}
				schedmap = pass_prio(sem->owndby, rt_current);
			} else {
				schedmap = 0;
			}	
			sem->count--;
			rt_current->state |= (SEMAPHORE | DELAYED);
			rem_ready_current(rt_current);
			enqueue_blocked(rt_current, &sem->queue, sem->qtype);
			enq_timed_task(rt_current);
			RT_SCHEDULE_MAP_BOTH(schedmap);
		} else {
			sem->count--;
			rt_current->queue.prev = rt_current->queue.next = &rt_current->queue;
		}
		if (sem->magic != RT_SEM_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return SEM_ERR;
		} else {
			if (rt_current->blocked_on) {
				dequeue_blocked(rt_current);
				if(++sem->count > 1 && sem->type) {
					sem->count = 1;
				}
				rt_global_restore_flags(flags);
				return SEM_TIMOUT;
			} else {
				count = sem->count;
			}
		}
	} else {
		sem->count--;
	}
	if (sem->type > 0) {
		(sem->owndby = rt_current)->owndres++;
	}
	rt_global_restore_flags(flags);
	return count;
}


int rt_sem_wait_timed(SEM *sem, RTIME delay)
{
	return rt_sem_wait_until(sem, get_time() + delay);
}


int rt_sem_wait_barrier(SEM *sem)
{
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (!sem->owndby) {
		sem->owndby = (void *)(sem->count < 1 ? 1 : sem->count);
		sem->count = sem->type = 0;
	}
	if ((1 - sem->count) < (int)sem->owndby) {
		rt_sem_wait(sem);
	} else {
		rt_sem_broadcast(sem);
	}
	rt_global_restore_flags(flags);
	return 0;
}

/* ++++++++++++++++++++++++++++++ MESSAGES +++++++++++++++++++++++++++++++++ */

/* ++++++++++++++++++++++++++++++++ SEND +++++++++++++++++++++++++++++++++++ */

RT_TASK *rt_send(RT_TASK *task, unsigned int msg)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RECEIVE) &&
	      (!task->msg_queue.task || task->msg_queue.task == rt_current)) {
		task->msg = msg;
		task->msg_queue.task = rt_current;
		task->ret_queue.task = NOTHING;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
			RT_SCHEDULE(task, cpuid);
		}
	} else {
		rt_current->msg = msg;
		rt_current->msg_queue.task = task;
		enqueue_blocked(rt_current, &task->msg_queue, 0);
		rt_current->state |= SEND;
		rem_ready_current(rt_current);
		rt_schedule();
	}
	if (rt_current->msg_queue.task != rt_current) {
		rt_current->msg_queue.task = rt_current;
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}


RT_TASK *rt_send_if(RT_TASK *task, unsigned int msg)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RECEIVE) &&
	      (!task->msg_queue.task || task->msg_queue.task == rt_current)) {
		task->msg = msg;
		task->msg_queue.task = rt_current;
		task->ret_queue.task = NOTHING;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
			RT_SCHEDULE(task, cpuid);
		}
		if (rt_current->msg_queue.task != rt_current) {
			rt_current->msg_queue.task = rt_current;
			task = (RT_TASK *)0;
		}
	} else {
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}


RT_TASK *rt_send_until(RT_TASK *task, unsigned int msg, RTIME time)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RECEIVE) &&
	      (!task->msg_queue.task || task->msg_queue.task == rt_current)) {
		task->msg = msg;
		task->msg_queue.task = rt_current;
		task->ret_queue.task = NOTHING;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
			RT_SCHEDULE(task, cpuid);
		}
	} else {
		rt_current->msg_queue.task = task;
		if ((rt_current->resume_time = time) > rt_time_h) {
			rt_current->msg = msg;
			enqueue_blocked(rt_current, &task->msg_queue, 0);
			rt_current->state |= (SEND | DELAYED);
			rem_ready_current(rt_current);
			enq_timed_task(rt_current);
			rt_schedule();
		} else {
			rt_current->queue.prev = rt_current->queue.next = &rt_current->queue;
		}
	}
	if (rt_current->msg_queue.task != rt_current) {
		dequeue_blocked(rt_current);
		rt_current->msg_queue.task = rt_current;
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}


RT_TASK *rt_send_timed(RT_TASK *task, unsigned int msg, RTIME delay)
{
	return rt_send_until(task, msg, get_time() + delay);
}

/* +++++++++++++++++++++++++++++++++ RPC +++++++++++++++++++++++++++++++++++ */

RT_TASK *rt_rpc(RT_TASK *task, unsigned int to_do, unsigned int *result)
{

	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RECEIVE) &&
		(!task->msg_queue.task || task->msg_queue.task == rt_current)) {
		rt_current->msg = task->msg = to_do;
		task->msg_queue.task = rt_current;
		task->ret_queue.task = NOTHING;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
		}
		rt_current->state |= RETURN;
	} else {
		rt_current->msg = to_do;
		task->owndres += RPCINC;
                pass_prio(task, rt_current);
                enqueue_blocked(rt_current, &task->msg_queue, 0);
		rt_current->state |= RPC;
	}
	rem_ready_current(rt_current);
	rt_current->msg_queue.task = task;
	RT_SCHEDULE_BOTH(task, cpuid);
	if (rt_current->msg_queue.task == rt_current) {
		*result = rt_current->msg;
	} else {
		rt_current->msg_queue.task = rt_current;
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}


RT_TASK *rt_rpc_if(RT_TASK *task, unsigned int to_do, unsigned int *result)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RECEIVE) &&
	      (!task->msg_queue.task || task->msg_queue.task == rt_current)) {
		rt_current->msg = task->msg = to_do;
		task->msg_queue.task = rt_current;
		task->ret_queue.task = NOTHING;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
		}
		rt_current->state |= RETURN;
		rem_ready_current(rt_current);
		rt_current->msg_queue.task = task;
		RT_SCHEDULE_BOTH(task, cpuid);
		if (rt_current->msg_queue.task == rt_current) {
			*result = rt_current->msg;
		} else {
			rt_current->msg_queue.task = rt_current;
			task = (RT_TASK *)0;
		}
	} else {
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}


RT_TASK *rt_rpc_until(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME time)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RECEIVE) &&
	    (!task->msg_queue.task || task->msg_queue.task == rt_current)) {
		rt_current->msg = task->msg = to_do;
		task->msg_queue.task = rt_current;
		task->ret_queue.task = NOTHING;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
		}
		rt_current->state |= (RETURN | DELAYED);
	} else {
		if ((rt_current->resume_time = time) <= rt_time_h) {
			rt_global_restore_flags(flags);
			return (RT_TASK *)0;
		}
		rt_current->msg = to_do;
		task->owndres += RPCINC;
		pass_prio(task, rt_current);
		enqueue_blocked(rt_current, &task->msg_queue, 0);
		rt_current->state |= (RPC | DELAYED);
	}
	rem_ready_current(rt_current);
	rt_current->msg_queue.task = task;
	enq_timed_task(rt_current);
	RT_SCHEDULE_BOTH(task, cpuid);
	if (rt_current->msg_queue.task == rt_current) {
		*result = rt_current->msg;
	} else {
		dequeue_blocked(rt_current);
		rt_current->msg_queue.task = rt_current;
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}


RT_TASK *rt_rpc_timed(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME delay)
{
	return rt_rpc_until(task, to_do, result, get_time() + delay);
}

/* ++++++++++++++++++++++++++++++ RPC_RETURN +++++++++++++++++++++++++++++++ */

int rt_isrpc(RT_TASK *task)
{
	return task->state & RETURN;
}


RT_TASK *rt_return(RT_TASK *task, unsigned int result)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((task->state & RETURN) && task->msg_queue.task == rt_current) {
		int sched;
		dequeue_blocked(task);
		if (rt_current->owndres & RPCHLF) {
			rt_current->owndres -= RPCINC;
		}
		if (!rt_current->owndres) {
			sched = renq_current(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_current(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
		} else {
			sched = 0;
		}
		task->msg = result;
		task->msg_queue.task = task;
		rem_timed_task(task);
		if (task->state != READY && (task->state &= ~(RETURN | DELAYED)) == READY) {
			enq_ready_task(task);
			if (sched) {
				RT_SCHEDULE_BOTH(task, cpuid);
			} else {
				RT_SCHEDULE(task, cpuid);
			}
                } else if (sched) {
                        rt_schedule();
                }
	} else {
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	return task;
}

/* +++++++++++++++++++++++++++++++ RECEIVE +++++++++++++++++++++++++++++++++ */

RT_TASK *rt_evdrp(RT_TASK *task, unsigned int *msg)
{
	DECLARE_RT_CURRENT;

	if (task && task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	ASSIGN_RT_CURRENT;
	if (!task) task = (rt_current->msg_queue.next)->task;
	if ((task->state & (SEND | RPC)) && task->msg_queue.task == rt_current) {
		*msg = task->msg;
	} else {
		task = (RT_TASK *)0;
	}
	return task;
}


RT_TASK *rt_receive(RT_TASK *task, unsigned int *msg)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task && task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (!task) task = (rt_current->msg_queue.next)->task;
	if ((task->state & (SEND | RPC)) && task->msg_queue.task == rt_current) {
		dequeue_blocked(task);
		rem_timed_task(task);
		*msg = task->msg;
		rt_current->msg_queue.task = task;
		if (task->state & SEND) {
			task->msg_queue.task = task;
			if (task->state != READY && (task->state &= ~(SEND | DELAYED)) == READY) {
				enq_ready_task(task);
				RT_SCHEDULE(task, cpuid);
			}
		} else if (task->state & RPC) {
                        enqueue_blocked(task, &rt_current->ret_queue, 0);
			task->state = (task->state & ~(RPC | DELAYED)) | RETURN;
		}
	} else {
		rt_current->ret_queue.task = SOMETHING;
		rt_current->state |= RECEIVE;
		rem_ready_current(rt_current);
		rt_current->msg_queue.task = task != rt_current ? task : (RT_TASK *)0;
		rt_schedule();
		*msg = rt_current->msg;
	}
	if (rt_current->ret_queue.task) {
		rt_current->ret_queue.task = NOTHING;
		task = (RT_TASK *)0;
	} else {
		task = rt_current->msg_queue.task;
	}
	rt_current->msg_queue.task = rt_current;
	rt_global_restore_flags(flags);
	if (task && (struct proxy_t *)task->stack_bottom) {
		if (((struct proxy_t *)task->stack_bottom)->receiver == rt_current) {
			rt_return(task, 0);
		}
	}
	return task;
}


RT_TASK *rt_receive_if(RT_TASK *task, unsigned int *msg)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task && task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (!task) task = (rt_current->msg_queue.next)->task;
	if ((task->state & (SEND | RPC)) && task->msg_queue.task == rt_current) {
		dequeue_blocked(task);
		rem_timed_task(task);
		*msg = task->msg;
		rt_current->msg_queue.task = task;
		if (task->state & SEND) {
			task->msg_queue.task = task;
			if (task->state != READY && (task->state &= ~(SEND | DELAYED)) == READY) {
				enq_ready_task(task);
				RT_SCHEDULE(task, cpuid);
			}
		} else if (task->state & RPC) {
			enqueue_blocked(task, &rt_current->ret_queue, 0);
			task->state = (task->state & ~(RPC | DELAYED)) | RETURN;
		}
		if (rt_current->ret_queue.task) {
			rt_current->ret_queue.task = NOTHING;
			task = (RT_TASK *)0;
		} else {
			task = rt_current->msg_queue.task;
		}
		rt_current->msg_queue.task = rt_current;
	} else {
		task = (RT_TASK *)0;
	}
	rt_global_restore_flags(flags);
	if (task && (struct proxy_t *)task->stack_bottom) {
		if (((struct proxy_t *)task->stack_bottom)->receiver == rt_current) {
			rt_return(task, 0);
		}
	}
	return task;
}


RT_TASK *rt_receive_until(RT_TASK *task, unsigned int *msg, RTIME time)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;

	if (task && task->magic != RT_TASK_MAGIC) {
		return MSG_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if (!task) task = (rt_current->msg_queue.next)->task;
	if ((task->state & (SEND | RPC)) && task->msg_queue.task == rt_current) {
		dequeue_blocked(task);
		rem_timed_task(task);
		*msg = task->msg;
		rt_current->msg_queue.task = task;
		if (task->state & SEND) {
			task->msg_queue.task = task;
			if (task->state != READY && (task->state &= ~(SEND | DELAYED)) == READY) {
				enq_ready_task(task);
				RT_SCHEDULE(task, cpuid);
			}
		} else if (task->state & RPC) {
			enqueue_blocked(task, &rt_current->ret_queue, 0);
			task->state = (task->state & ~(RPC | DELAYED)) | RETURN;
		}
	} else {
		rt_current->ret_queue.task = SOMETHING;
		if ((rt_current->resume_time = time) > rt_time_h) {
			rt_current->state |= (RECEIVE | DELAYED);
			rem_ready_current(rt_current);
			rt_current->msg_queue.task = task != rt_current ? task : (RT_TASK *)0;
			enq_timed_task(rt_current);
			rt_schedule();
			*msg = rt_current->msg;
		}
	}
	if (rt_current->ret_queue.task) {
		rt_current->ret_queue.task = NOTHING;
		task = (RT_TASK *)0;
	} else {
		task = rt_current->msg_queue.task;
	}
	rt_current->msg_queue.task = rt_current;
	rt_global_restore_flags(flags);
	if (task && (struct proxy_t *)task->stack_bottom) {
		if (((struct proxy_t *)task->stack_bottom)->receiver == rt_current) {
			rt_return(task, 0);
		}
	}
	return task;
}


RT_TASK *rt_receive_timed(RT_TASK *task, unsigned int *msg, RTIME delay)
{
	return rt_receive_until(task, msg, get_time() + delay);
}

/* ++++++++++++++++++++++++++ EXTENDED MESSAGES +++++++++++++++++++++++++++++++
COPYRIGHT (C) 2002  Pierre Cloutier  (pcloutier@poseidoncontrols.com)
                    Paolo Mantegazza (mantegazza@aero.polimi.it)
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#define SET_RPC_MCB() \
	do { \
		mcb.sbuf   = smsg; \
		mcb.sbytes = ssize; \
		mcb.rbuf   = rmsg; \
		mcb.rbytes = rsize; \
	} while (0)

RT_TASK *rt_rpcx(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (task) {
		struct mcb_t mcb;
		SET_RPC_MCB();
		return rt_rpc(task, (unsigned int)&mcb, &mcb.rbytes);
	}
	return 0;
}


RT_TASK *rt_rpcx_if(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (task) {
		struct mcb_t mcb;
		SET_RPC_MCB();
		return rt_rpc_if(task, (unsigned int)&mcb, &mcb.rbytes);
	}
	return 0;
}


RT_TASK *rt_rpcx_until(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time)
{
	if (task) {
		struct mcb_t mcb;
		SET_RPC_MCB();
		return rt_rpc_until(task, (unsigned int)&mcb, &mcb.rbytes, time);
	}
	return 0;
}


RT_TASK *rt_rpcx_timed(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay)
{
	if (task) {
		struct mcb_t mcb;
		SET_RPC_MCB();
		return rt_rpc_timed(task, (unsigned int)&mcb, &mcb.rbytes, delay);
	}
	return 0;
}

#define task_mcb (task->mcb)
#define SET_SEND_MCB() \
	do { \
		task_mcb.sbuf   = msg; \
		task_mcb.sbytes = size; \
		task_mcb.rbuf   = 0; \
		task_mcb.rbytes = 0; \
	} while (0)

RT_TASK *rt_sendx(RT_TASK *task, void *msg, int size) 
{
	if (task) {
		SET_SEND_MCB();
		return rt_send(task, (unsigned int)&task_mcb);
	}
	return 0;
}


RT_TASK *rt_sendx_if(RT_TASK *task, void *msg, int size)
{
	if (task) {
		SET_SEND_MCB();
		return rt_send_if(task, (unsigned int)&task_mcb);
	}
	return 0;
}


RT_TASK *rt_sendx_until(RT_TASK *task, void *msg, int size, RTIME time)
{
	if (task) {
		SET_SEND_MCB();
		return rt_send_until(task, (unsigned int)&task_mcb, time);
	}
	return 0;
}


RT_TASK *rt_sendx_timed(RT_TASK *task, void *msg, int size, RTIME delay)
{
	if (task) {
		SET_SEND_MCB();
		return rt_send_timed(task, (unsigned int)&task_mcb, delay);
	}
	return 0;
}


RT_TASK *rt_returnx(RT_TASK *task, void *msg, int size)
{
	if (task) {
		struct mcb_t *mcb;
		if ((mcb = (struct mcb_t *)task->msg)->rbytes < size) {
			size = mcb->rbytes;
		}
		if (size) {
			memcpy(mcb->rbuf, msg, size);
		}
		return rt_return(task, 0);
	}
	return 0;
}

#define DO_RCV_MSG() \
	do { \
		if ((*len = size <= mcb->sbytes ? size : mcb->sbytes)) { \
			memcpy(msg, mcb->sbuf, *len); \
		} \
	} while (0)


RT_TASK *rt_evdrpx(RT_TASK *task, void *msg, int size, int *len)
{
	struct mcb_t *mcb;
	if ((task = rt_evdrp(task, (unsigned int *)&mcb))) {
		DO_RCV_MSG();
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex(RT_TASK *task, void *msg, int size, int *len)
{
	struct mcb_t *mcb;
	if ((task = rt_receive(task, (unsigned int *)&mcb))) {
		DO_RCV_MSG();
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex_if(RT_TASK *task, void *msg, int size, int *len)
{
	struct mcb_t *mcb;
	if ((task = rt_receive_if(task, (unsigned int *)&mcb))) {
		DO_RCV_MSG();
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex_until(RT_TASK *task, void *msg, int size, int *len, RTIME time)
{
	struct mcb_t *mcb;
	if ((task = rt_receive_until(task, (unsigned int *)&mcb, time))) {
		DO_RCV_MSG();
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex_timed(RT_TASK *task, void *msg, int size, int *len, RTIME delay)
{
	struct mcb_t *mcb;
	if ((task = rt_receive_timed(task, (unsigned int *)&mcb, delay))) {
		DO_RCV_MSG();
		return task;
	}
	return 0;
}
/* +++++++++++++++++++++++++++++ MAIL BOXES ++++++++++++++++++++++++++++++++ */

static inline void mbx_signal(MBX *mbx)
{
	unsigned long flags;
	RT_TASK *task;
	int tosched;

	flags = rt_global_save_flags_and_cli();
	if ((task = mbx->waiting_task)) {
		rem_timed_task(task);
		mbx->waiting_task = NOTHING;
		task->prio_passed_to = NOTHING;
		if (task->state != READY && (task->state &= ~(MBXSUSP | DELAYED)) == READY) {
			enq_ready_task(task);
			if (mbx->sndsem.type <= 0) {
				RT_SCHEDULE(task, hard_cpu_id());
				rt_global_restore_flags(flags);
				return;
			}
			tosched = 1;
			goto res;
		}
	}
	tosched = 0;
res:	if (mbx->sndsem.type > 0) {
		DECLARE_RT_CURRENT;
		int sched;
		ASSIGN_RT_CURRENT;
		mbx->owndby = 0;
		if (rt_current->owndres & SEMHLF) {
			--rt_current->owndres;
		}
		if (!rt_current->owndres) {
			sched = renq_current(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_current(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
		} else {
			sched = 0;
		}
		if (rt_current->suspdepth) {
			if (rt_current->suspdepth > 0) {
				rt_current->state |= SUSPENDED;
				rem_ready_current(rt_current);
                        	sched = 1;
			} else {
				rt_task_delete(rt_current);
			}
		}
		if (sched) {
			if (tosched) {
				RT_SCHEDULE_BOTH(task, cpuid);
			} else {
				rt_schedule();
			}
		} else if (tosched) {
			RT_SCHEDULE(task, cpuid);
		}
	}
	rt_global_restore_flags(flags);
}

#define RT_MBX_MAGIC 0x3ad46e9b

static inline int mbx_wait(MBX *mbx, int *fravbs, RT_TASK *rt_current)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (!(*fravbs)) {
		unsigned long schedmap;
		if (mbx->sndsem.type > 0) {
			schedmap = pass_prio(mbx->owndby, rt_current);
		} else {
			schedmap = 0;
		}
		rt_current->state |= MBXSUSP;
		rem_ready_current(rt_current);
		mbx->waiting_task = rt_current;
		RT_SCHEDULE_MAP_BOTH(schedmap);
		if (mbx->waiting_task == rt_current || mbx->magic != RT_MBX_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return -1;
		}
	}
	if (mbx->sndsem.type > 0) {
		(mbx->owndby = rt_current)->owndres++;
	}
	rt_global_restore_flags(flags);
	return 0;
}

static inline int mbx_wait_until(MBX *mbx, int *fravbs, RTIME time, RT_TASK *rt_current)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (!(*fravbs)) {
		mbx->waiting_task = rt_current;
		if ((rt_current->resume_time = time) > rt_smp_time_h[hard_cpu_id()]) {
			unsigned long schedmap;
			if (mbx->sndsem.type > 0) {
				schedmap = pass_prio(mbx->owndby, rt_current);
			} else {
				schedmap = 0;
			}
			rt_current->state |= (MBXSUSP | DELAYED);
			rem_ready_current(rt_current);
			enq_timed_task(rt_current);
			RT_SCHEDULE_MAP_BOTH(schedmap);
		}
		if (mbx->magic != RT_MBX_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return -1;
		}
		if (mbx->waiting_task == rt_current) {
			mbx->waiting_task = NOTHING;
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return -1;
		}
	}
	if (mbx->sndsem.type > 0) {
		(mbx->owndby = rt_current)->owndres++;
	}
	rt_global_restore_flags(flags);
	return 0;
}

#define MOD_SIZE(indx) ((indx) < mbx->size ? (indx) : (indx) - mbx->size)

static inline int mbxput(MBX *mbx, char **msg, int msg_size)
{
	unsigned long flags;
	int tocpy;

	while (msg_size > 0 && mbx->frbs) {
		if ((tocpy = mbx->size - mbx->lbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > mbx->frbs) {
			tocpy = mbx->frbs;
		}
		memcpy(mbx->bufadr + mbx->lbyte, *msg, tocpy);
		flags = rt_spin_lock_irqsave(&(mbx->lock));
		mbx->frbs -= tocpy;
		mbx->avbs += tocpy;
		rt_spin_unlock_irqrestore(flags, &(mbx->lock));
		msg_size -= tocpy;
		*msg     += tocpy;
		mbx->lbyte = MOD_SIZE(mbx->lbyte + tocpy);
	}
	return msg_size;
}

static inline int mbxovrwrput(MBX *mbx, char **msg, int msg_size)
{
	unsigned long flags;
	int tocpy,n;

	if ((n = msg_size - mbx->size) > 0) {
		*msg += n;
		msg_size -= n;
	}		
	while (msg_size > 0) {
		if (mbx->frbs) {	
			if ((tocpy = mbx->size - mbx->lbyte) > msg_size) {
				tocpy = msg_size;
			}
			if (tocpy > mbx->frbs) {
				tocpy = mbx->frbs;
			}
			memcpy(mbx->bufadr + mbx->lbyte, *msg, tocpy);
	        	flags = rt_spin_lock_irqsave(&(mbx->lock));
			mbx->frbs -= tocpy;
			mbx->avbs += tocpy;
        		rt_spin_unlock_irqrestore(flags, &(mbx->lock));
			msg_size -= tocpy;
			*msg     += tocpy;
			mbx->lbyte = MOD_SIZE(mbx->lbyte + tocpy);
		}	
		if (msg_size) {
			while ((n = msg_size - mbx->frbs) > 0) {
				if ((tocpy = mbx->size - mbx->fbyte) > n) {
					tocpy = n;
				}
				if (tocpy > mbx->avbs) {
					tocpy = mbx->avbs;
				}
	        		flags = rt_spin_lock_irqsave(&(mbx->lock));
				mbx->frbs  += tocpy;
				mbx->avbs  -= tocpy;
        			rt_spin_unlock_irqrestore(flags, &(mbx->lock));
				mbx->fbyte = MOD_SIZE(mbx->fbyte + tocpy);
			}
		}		
	}
	return 0;
}

static inline int mbxget(MBX *mbx, char **msg, int msg_size)
{
	unsigned long flags;
	int tocpy;

	while (msg_size > 0 && mbx->avbs) {
		if ((tocpy = mbx->size - mbx->fbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > mbx->avbs) {
			tocpy = mbx->avbs;
		}
		memcpy(*msg, mbx->bufadr + mbx->fbyte, tocpy);
		flags = rt_spin_lock_irqsave(&(mbx->lock));
		mbx->frbs  += tocpy;
		mbx->avbs  -= tocpy;
		rt_spin_unlock_irqrestore(flags, &(mbx->lock));
		msg_size -= tocpy;
		*msg     += tocpy;
		mbx->fbyte = MOD_SIZE(mbx->fbyte + tocpy);
	}
	return msg_size;
}

static inline int mbxevdrp(MBX *mbx, char **msg, int msg_size)
{
	int tocpy, fbyte, avbs;

	fbyte = mbx->fbyte;
	avbs  = mbx->avbs;
	while (msg_size > 0 && avbs) {
		if ((tocpy = mbx->size - fbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > avbs) {
			tocpy = avbs;
		}
		memcpy(*msg, mbx->bufadr + fbyte, tocpy);
		avbs     -= tocpy;
		msg_size -= tocpy;
		*msg     += tocpy;
		fbyte = MOD_SIZE(fbyte + tocpy);
	}
	return msg_size;
}

int rt_mbx_evdrp(MBX *mbx, void *msg, int msg_size)
{
	return mbxevdrp(mbx, (char **)(&msg), msg_size);
}

#define CHK_MBX_MAGIC { if (mbx->magic != RT_MBX_MAGIC) { return -EINVAL; } }

int rt_typed_mbx_init(MBX *mbx, int size, int type)
{
	if (!(mbx->bufadr = sched_malloc(size))) { 
		return -ENOMEM;
	}
	rt_typed_sem_init(&(mbx->sndsem), 1, type & 3 ? type : BIN_SEM | type);
	rt_typed_sem_init(&(mbx->rcvsem), 1, type & 3 ? type : BIN_SEM | type);
	mbx->magic = RT_MBX_MAGIC;
	mbx->size = mbx->frbs = size;
	mbx->waiting_task = mbx->owndby = 0;
	mbx->fbyte = mbx->lbyte = mbx->avbs = 0;
        spin_lock_init(&(mbx->lock));
	return 0;
}


int rt_mbx_init(MBX *mbx, int size)
{
	return rt_typed_mbx_init(mbx, size, PRIO_Q);
}


int rt_mbx_delete(MBX *mbx)
{
	CHK_MBX_MAGIC;
	mbx->magic = 0;
	if (rt_sem_delete(&mbx->sndsem) || rt_sem_delete(&mbx->rcvsem)) {
		return -EFAULT;
	}
	while (mbx->waiting_task) {
		mbx_signal(mbx);
	}
	sched_free(mbx->bufadr); 
	return 0;
}


int rt_mbx_send(MBX *mbx, void *msg, int msg_size)
{
	DECLARE_RT_CURRENT;

	CHK_MBX_MAGIC;
	if (rt_sem_wait(&mbx->sndsem) > 1) {
		return msg_size;
	}
	ASSIGN_RT_CURRENT;
	while (msg_size) {
		if (mbx_wait(mbx, &mbx->frbs, rt_current)) {
			rt_sem_signal(&mbx->sndsem);
			return msg_size;
		}
		msg_size = mbxput(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
	}
	rt_sem_signal(&mbx->sndsem);
	return 0;
}


int rt_mbx_send_wp(MBX *mbx, void *msg, int msg_size)
{
	unsigned long flags;

	CHK_MBX_MAGIC;
	flags = rt_global_save_flags_and_cli();
	if (mbx->sndsem.count && mbx->frbs) {
		mbx->sndsem.count = 0;
		if (mbx->sndsem.type > 0) {
			(mbx->sndsem.owndby = mbx->owndby = RT_CURRENT)->owndres += 2;
		}
		rt_global_restore_flags(flags);
		msg_size = mbxput(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
		rt_sem_signal(&mbx->sndsem);
	} else {
		rt_global_restore_flags(flags);
	}
	return msg_size;
}


int rt_mbx_send_if(MBX *mbx, void *msg, int msg_size)
{
	unsigned long flags;

	CHK_MBX_MAGIC;
	flags = rt_global_save_flags_and_cli();
	if (mbx->sndsem.count && msg_size <= mbx->frbs) {
		mbx->sndsem.count = 0;
		if (mbx->sndsem.type > 0) {
			(mbx->sndsem.owndby = mbx->owndby = RT_CURRENT)->owndres += 2;
		}
		rt_global_restore_flags(flags);
		mbxput(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
		rt_sem_signal(&mbx->sndsem);
		return 0;
	}
	rt_global_restore_flags(flags);
	return msg_size;
}


int rt_mbx_send_until(MBX *mbx, void *msg, int msg_size, RTIME time)
{
	DECLARE_RT_CURRENT;

	CHK_MBX_MAGIC;
	if (rt_sem_wait_until(&mbx->sndsem, time) > 1) {
		return msg_size;
	}
	ASSIGN_RT_CURRENT;
	while (msg_size) {
		if (mbx_wait_until(mbx, &mbx->frbs, time, rt_current)) {
			rt_sem_signal(&mbx->sndsem);
			return msg_size;
		}
		msg_size = mbxput(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
	}
	rt_sem_signal(&mbx->sndsem);
	return 0;
}


int rt_mbx_send_timed(MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	int cpuid = hard_cpu_id();
	return rt_mbx_send_until(mbx, msg, msg_size, (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay);
}


int rt_mbx_receive(MBX *mbx, void *msg, int msg_size)
{
	DECLARE_RT_CURRENT;

	CHK_MBX_MAGIC;
	if (rt_sem_wait(&mbx->rcvsem) > 1) {
		return msg_size;
	}
	ASSIGN_RT_CURRENT;
	while (msg_size) {
		if (mbx_wait(mbx, &mbx->avbs, rt_current)) {
			rt_sem_signal(&mbx->rcvsem);
			return msg_size;
		}
		msg_size = mbxget(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
	}
	rt_sem_signal(&mbx->rcvsem);
	return 0;
}


int rt_mbx_receive_wp(MBX *mbx, void *msg, int msg_size)
{
	unsigned long flags;

	CHK_MBX_MAGIC;
	flags = rt_global_save_flags_and_cli();
	if (mbx->rcvsem.count && mbx->avbs) {
		mbx->rcvsem.count = 0;
		if (mbx->rcvsem.type > 0) {
			(mbx->rcvsem.owndby = mbx->owndby = RT_CURRENT)->owndres += 2;
		}
		rt_global_restore_flags(flags);
		msg_size = mbxget(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
		rt_sem_signal(&mbx->rcvsem);
	} else {
		rt_global_restore_flags(flags);
	}
	return msg_size;
}


int rt_mbx_receive_if(MBX *mbx, void *msg, int msg_size)
{
	unsigned long flags;

	CHK_MBX_MAGIC;
	flags = rt_global_save_flags_and_cli();
	if (mbx->rcvsem.count && msg_size <= mbx->avbs) {
		mbx->rcvsem.count = 0;
		if (mbx->rcvsem.type > 0) {
			(mbx->rcvsem.owndby = mbx->owndby = RT_CURRENT)->owndres += 2;
		}
		rt_global_restore_flags(flags);
		mbxget(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
		rt_sem_signal(&mbx->rcvsem);
		return 0;
	}
	rt_global_restore_flags(flags);
	return msg_size;
}


int rt_mbx_receive_until(MBX *mbx, void *msg, int msg_size, RTIME time)
{
	DECLARE_RT_CURRENT;

	CHK_MBX_MAGIC;
	if (rt_sem_wait_until(&mbx->rcvsem, time) > 1) {
		return msg_size;
	}
	ASSIGN_RT_CURRENT;
	while (msg_size) {
		if (mbx_wait_until(mbx, &mbx->avbs, time, rt_current)) {
			rt_sem_signal(&mbx->rcvsem);
			return msg_size;
		}
		msg_size = mbxget(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
	}
	rt_sem_signal(&mbx->rcvsem);
	return 0;
}


int rt_mbx_receive_timed(MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	int cpuid = hard_cpu_id();
	return rt_mbx_receive_until(mbx, msg, msg_size, (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay);
}


int rt_mbx_ovrwr_send(MBX *mbx, void *msg, int msg_size)
{
	unsigned long flags;

	CHK_MBX_MAGIC;

	flags = rt_global_save_flags_and_cli();
	if (mbx->sndsem.count) {
		mbx->sndsem.count = 0;
		if (mbx->sndsem.type > 0) {
			(mbx->sndsem.owndby = mbx->owndby = RT_CURRENT)->owndres += 2;
		}
		rt_global_restore_flags(flags);
		msg_size = mbxovrwrput(mbx, (char **)(&msg), msg_size);
		mbx_signal(mbx);
		rt_sem_signal(&mbx->sndsem);
	} else {
		rt_global_restore_flags(flags);
	}
	return msg_size;
}

/* +++++++++++++++++++++++++++ SECRET BACK DOORS ++++++++++++++++++++++++++++ */

void (*dnepsus_trxl)(void) = rt_schedule;

void emuser_trxl(RT_TASK *new_task) { }

RT_TASK *rt_get_base_linux_task(RT_TASK **base_linux_tasks)
{
        int cpuid;
        for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
                base_linux_tasks[cpuid] = rt_smp_linux_task + cpuid;
        }
        return rt_smp_linux_task;
}

RT_TASK *rt_alloc_dynamic_task(void)
{
#if defined(CONFIG_RTAI_DYN_MM) || defined(CONFIG_RTAI_DYN_MM_MODULE)
        return rt_malloc(sizeof(RT_TASK)); // For VC's, proxies and C++ support.
#else
	return NULL;
#endif
}

/* +++++++++++++++++++++++++++ WATCHDOG SUPPORT ++++++++++++++++++++++++++++ */

RT_TASK **rt_register_watchdog(RT_TASK *wd, int cpuid)
{
    	RT_TASK *task;

	if (wdog_task[cpuid]) return (RT_TASK**) -EBUSY;
	task = &rt_linux_task;
	while ((task = task->next)) {
		if (task != wd && task->priority == RT_HIGHEST_PRIORITY) {
			return (RT_TASK**) -EBUSY;
		}
	}
	wdog_task[cpuid] = wd;
	return (RT_TASK**) 0;
}

void rt_deregister_watchdog(RT_TASK *wd, int cpuid)
{
    	if (wdog_task[cpuid] != wd) return;
	wdog_task[cpuid] = NULL;
}

/* +++++++++++++++++++ READY AND TIMED QUEUE MANIPULATION +++++++++++++++++++ */

void rt_enq_ready_edf_task(RT_TASK *ready_task)
{
	enq_ready_edf_task(ready_task);
}

void rt_enq_ready_task(RT_TASK *ready_task)
{
	enq_ready_task(ready_task);
}

int rt_renq_ready_task(RT_TASK *ready_task, int priority)
{
	return renq_ready_task(ready_task, priority);
}

int rt_renq_current(RT_TASK *rt_current, int priority)
{
	return renq_current(rt_current, priority);
}

void rt_rem_ready_task(RT_TASK *task)
{
	rem_ready_task(task);
}

void rt_rem_ready_current(RT_TASK *rt_current)
{
	rem_ready_current(rt_current);
}

void rt_enq_timed_task(RT_TASK *timed_task)
{
	enq_timed_task(timed_task);
}

void rt_wake_up_timed_tasks(int cpuid)
{
	wake_up_timed_tasks(cpuid);
}

void rt_rem_timed_task(RT_TASK *task)
{
	rem_timed_task(task);
}

void rt_dequeue_blocked(RT_TASK *task)
{
	dequeue_blocked(task);
}

/* +++++++++++++++ SUPPORT FOR LINUX TASKS AND KERNEL THREADS +++++++++++++++ */

//#define ECHO_SYSW
#ifdef ECHO_SYSW
#define SYSW_DIAG_MSG(x) x
#else
#define SYSW_DIAG_MSG(x)
#endif

extern volatile unsigned long lxrt_hrt_flags; 

static RT_TRAP_HANDLER sched_trap_handler;

#ifdef CONFIG_RTAI_FPU_SUPPORT
static void init_fpu(struct task_struct *tsk)
{
        init_xfpu();
        tsk->used_math = 1;
        tsk->flags |= PF_USEDFPU;
}
#else
static void init_fpu(struct task_struct *tsk) { }
#endif

static void linux_lxrt_global_cli(void)
{
	hard_cli();
}

static inline void fast_schedule(RT_TASK *new_task)
{
	struct task_struct *prev;
	int cpuid;
	if (((new_task)->state |= READY) == READY) {
		enq_ready_task(new_task);
		rt_release_global_lock();
		LOCK_LINUX(cpuid = hard_cpu_id());
		rt_linux_task.lnxtsk = prev = current;
#define rt_current (rt_smp_current[cpuid])
		UEXECTIME();
#undef rt_current
		rt_smp_current[cpuid] = new_task;
		TASK_SWITCH(prev, new_task->lnxtsk, cpuid);
		if (prev->used_math) {
			restore_fpu(prev);
		}
		UNLOCK_LINUX(cpuid);
	}
}

struct fun_args { int a0; int a1; int a2; int a3; int a4; int a5; int a6; int a7; int a8; int a9; long long (*fun)(int, ...); };

void rt_schedule_soft(RT_TASK *rt_task)
{
	struct fun_args *funarg;
	int cpuid, priority, rt_priority, policy;

	if ((priority = rt_task->priority) < BASE_SOFT_PRIORITY) {
		rt_task->priority += BASE_SOFT_PRIORITY;
	}
	rt_global_cli();
	rt_priority = current->rt_priority;
	current->rt_priority = BASE_SOFT_PRIORITY;
	policy = current->policy;
	current->policy = SCHED_FIFO;
	while (((rt_task)->state |= READY) != READY) {
		current->state = TASK_HARDREALTIME;
		rt_global_sti();
		schedule();
		rt_global_cli();
	}
	LOCK_LINUX(cpuid = hard_cpu_id());
	enq_ready_task(rt_task);
	rt_smp_current[cpuid] = rt_task;
	rt_global_sti();
	funarg = (void *)rt_task->fun_args;
	rt_task->retval = funarg->fun(funarg->a0, funarg->a1, funarg->a2, funarg->a3, funarg->a4, funarg->a5, funarg->a6, funarg->a7, funarg->a8, funarg->a9);
	rt_global_cli();
	if (current->rt_priority == BASE_SOFT_PRIORITY) {
		(rt_task->rprev)->rnext = rt_task->rnext;
        	(rt_task->rnext)->rprev = rt_task->rprev;
	}
	rt_task->state = 0;
	if (rt_smp_current[cpuid] == rt_task) {
		rt_smp_current[cpuid] = &rt_linux_task;
		rt_schedule();
	}
	UNLOCK_LINUX(cpuid);
	rt_task->priority = priority;
	current->rt_priority = rt_priority;
	current->policy = policy;
	rt_global_sti();
	schedule();
}

static struct klist_t klistb[NR_RT_CPUS];
static struct task_struct *kthreadb[NR_RT_CPUS];
static struct klist_t klistm[NR_RT_CPUS];
static struct task_struct *kthreadm[NR_RT_CPUS];
static struct semaphore resem[NR_RT_CPUS];
static int endkthread;

static void kthread_b(int cpuid)
{
	RT_TASK *rt_task;
	struct klist_t *klistp;

	sprintf(current->comm, "RTAI_KTHRD_B:%d", cpuid);
	put_current_on_cpu(cpuid);
	kthreadb[cpuid] = current;
	klistp = &klistb[cpuid];
	current->rt_priority = 100;
	current->policy = SCHED_FIFO;
	sigfillset(&current->blocked);
	up(&resem[cpuid]);
	while (!endkthread) {
		current->state = TASK_UNINTERRUPTIBLE;
		schedule();
		while (klistp->out != klistp->in) {
			rt_task = klistp->task[klistp->out];
			klistp->out = (klistp->out + 1) & (MAX_SRQ - 1);
			rt_global_cli();
			fast_schedule(rt_task);
			rt_global_sti();
		}
	}
	kthreadb[cpuid] = 0;
}

static RT_TASK thread_task[NR_RT_CPUS];
static int rsvr_cnt[NR_RT_CPUS];

#define RESERVOIR 4
static int Reservoir = RESERVOIR;
MODULE_PARM(Reservoir, "i");

static int taskidx[NR_RT_CPUS];
static struct task_struct **taskav[NR_RT_CPUS];

static struct task_struct *__get_kthread(int cpuid)
{
	unsigned long flags;
	struct task_struct *p;

	flags = rt_global_save_flags_and_cli();
	if (taskidx[cpuid] > 0) {
		p = taskav[cpuid][--taskidx[cpuid]];
		rt_global_restore_flags(flags);
		return p;
	}
	rt_global_restore_flags(flags);
	return 0;
}


static void thread_fun(int cpuid) 
{
	void steal_from_linux(RT_TASK *);
	void give_back_to_linux(RT_TASK *);
	RT_TASK *task;

	current->rt_priority = 100;
	current->policy = SCHED_FIFO;
	sprintf(current->comm, "F:HARD:%d:%d", cpuid, ++rsvr_cnt[cpuid]);
	current->this_rt_task[0] = task = &thread_task[cpuid];
	current->this_rt_task[1] = task->lnxtsk = current;
	sigfillset(&current->blocked);
	put_current_on_cpu(cpuid);
	steal_from_linux(task);
	init_fpu(current);
	rt_task_suspend(task);
	current->comm[0] = 'U';
	task = (RT_TASK *)current->this_rt_task[0];
	task->exectime[1] = rdtsc();
	((void (*)(int))task->max_msg_size[0])(task->max_msg_size[1]);
	rt_task_suspend(task);
}

static void kthread_m(int cpuid)
{
	struct task_struct *lnxtsk;
	struct klist_t *klistp;
	RT_TASK *task;

	(task = &thread_task[cpuid])->magic = RT_TASK_MAGIC;
	task->runnable_on_cpus = cpuid;
	sprintf(current->comm, "RTAI_KTHRD_M:%d", cpuid);
	put_current_on_cpu(cpuid);
	kthreadm[cpuid] = current;
	klistp = &klistm[cpuid];
	current->rt_priority = 101;
	current->policy = SCHED_FIFO;
	sigfillset(&current->blocked);
	up(&resem[cpuid]);
	while (!endkthread) {
		current->state = TASK_UNINTERRUPTIBLE;
		schedule();
		while (klistp->out != klistp->in) {
			unsigned long hard;
			rt_global_cli();
			rt_global_sti();
			hard = (unsigned long)(lnxtsk = klistp->task[klistp->out]);
			if (hard > 1) {
				lnxtsk->state = TASK_ZOMBIE;
				lnxtsk->exit_signal = SIGCHLD;
				waitpid(lnxtsk->pid, 0, 0);
			} else {
				rt_global_cli();
				if (taskidx[cpuid] < Reservoir) {
					rt_global_sti();
					task->suspdepth = task->state = 0;
					kernel_thread((void *)thread_fun, (void *)cpuid, 0);
					while (task->state != (READY | SUSPENDED)) {
						current->state = TASK_INTERRUPTIBLE;
						schedule_timeout(1);
					}
					rt_global_cli();
					taskav[cpuid][taskidx[cpuid]++] = (void *)task->lnxtsk;
				}
				rt_global_sti();
				klistp->out = (klistp->out + 1) & (MAX_SRQ - 1);
				if (hard) {
					rt_task_resume((void *)klistp->task[klistp->out]);
				} else {
					up(&resem[cpuid]);
				}
			}
			klistp->out = (klistp->out + 1) & (MAX_SRQ - 1);
		}
	}
	kthreadm[cpuid] = 0;
}

void steal_from_linux(RT_TASK *rt_task)
{
	int cpuid;
	struct klist_t *klistp;
	cpuid = rt_task->runnable_on_cpus;
	put_current_on_cpu(cpuid);
	klistp = &klistb[cpuid];
	hard_cli();
	klistp->task[klistp->in] = rt_task;
	klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
	hard_sti();
	current->state = TASK_HARDREALTIME | 0x80000000;
	wake_up_process(kthreadb[cpuid]);
	schedule();
	rt_task->is_hard = 1;
	rt_task->exectime[1] = rdtsc();
	current->state = TASK_HARDREALTIME;
	hard_sti();
	if (current->used_math) {
		restore_fpu(current);
	}
}

void give_back_to_linux(RT_TASK *rt_task)
{
	rt_global_cli();
	wake_up_srq.task[wake_up_srq.in] = rt_task->lnxtsk;;
	wake_up_srq.in = (wake_up_srq.in + 1) & (MAX_SRQ - 1);
	rt_pend_linux_srq(wake_up_srq.srq);
	(rt_task->rprev)->rnext = rt_task->rnext;
	(rt_task->rnext)->rprev = rt_task->rprev;
	rt_task->state = 0;
	rt_schedule();
	rt_global_sti();
	rt_task->is_hard = 0;
}


static struct task_struct *get_kthread(int get, int cpuid, void *lnxtsk)
{
	struct task_struct *kthread;
	struct klist_t *klistp;
	RT_TASK *this_task;
	int hard;

	klistp = &klistm[cpuid];
	if (get) {
		while (!(kthread = __get_kthread(cpuid))) {
			this_task = rt_smp_current[hard_cpu_id()];
			rt_global_cli();
			klistp->task[klistp->in] = (void *)(hard = this_task->is_hard > 0 ? 1 : 0);
			klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
			klistp->task[klistp->in] = (void *)this_task;
			klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
			wake_up_srq.task[wake_up_srq.in] = kthreadm[cpuid];
			wake_up_srq.in = (wake_up_srq.in + 1) & (MAX_SRQ - 1);
			rt_pend_linux_srq(wake_up_srq.srq);
			rt_global_sti();
        		if (hard) {
				rt_task_suspend(this_task);
			} else {
				down(&resem[cpuid]);
			}
		}
		rt_global_cli();
		klistp->task[klistp->in] = 0;
		klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
		klistp->task[klistp->in] = 0;
	} else {
		kthread = 0;
		rt_global_cli();
		klistp->task[klistp->in] = lnxtsk;
	}
	klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
	wake_up_srq.task[wake_up_srq.in] = kthreadm[cpuid];
	wake_up_srq.in = (wake_up_srq.in + 1) & (MAX_SRQ - 1);
	rt_pend_linux_srq(wake_up_srq.srq);
	rt_global_sti();
	return kthread;
}

static void start_stop_kthread(RT_TASK *task, void (*rt_thread)(int), int data, int priority, int uses_fpu, void(*signal)(void), int runnable_on_cpus)
{
	if (rt_thread) {
		task->retval = set_rtext(task, priority, uses_fpu, signal, runnable_on_cpus, get_kthread(1, runnable_on_cpus, 0));
		task->max_msg_size[0] = (int)rt_thread;
		task->max_msg_size[1] = data;
	} else {
		get_kthread(0, task->runnable_on_cpus, task->lnxtsk);
	}
}

static void wake_up_srq_handler(void)
{
        while (wake_up_srq.out != wake_up_srq.in) {
		wake_up_process(wake_up_srq.task[wake_up_srq.out]);
                wake_up_srq.out = (wake_up_srq.out + 1) & (MAX_SRQ - 1);
        }
	current->need_resched = 1;
}

DEFINE_LXRT_SYSCALL_HANDLER

static void (*linux_syscall_handler)(void);

static int inhrtp(unsigned long) __attribute__ ((__unused__));

static int inhrtp(unsigned long eax)
{
	if (test_bit(hard_cpu_id(), &lxrt_hrt_flags)) {
		RT_TASK *task;
		give_back_to_linux(task = current->this_rt_task[0]);
		task->is_hard = 2;
		SYSW_DIAG_MSG(rt_printk("FORCING IT SOFT, PID = %d, EIP = %lx.\n", current->pid, *(&eax - 1)););
	}
	return 0;
}

static int htrp7;

int rtai_trap_handler(int vec, int signo, struct pt_regs *regs, void *dummy_data)
{
#ifdef CONFIG_RTAI_ADEOS
	DECLARE_RT_CURRENT;
	ASSIGN_RT_CURRENT;

	if (rt_current->is_hard > 0 && rt_current->lnxtsk)
	    {
	    give_back_to_linux(rt_current);
	    rt_current->is_hard = 2;
	    }
	/* Exception #7 case is processed by ARTI. */
#else /* !CONFIG_RTAI_ADEOS */
{
	RT_TASK	*rt_task;
        DECLARE_RT_CURRENT;
        ASSIGN_RT_CURRENT;
        if (!rt_current->lnxtsk) {
	        if (!rt_current) return 0;
        	if (rt_current->task_trap_handler[vec]) {
                	return rt_current->task_trap_handler[vec]( vec,
                        	                                   signo,
                                	                           regs,
                                        	                   rt_current);
	        }
        	rt_printk("Default Trap Handler: vector %d: Suspend RT task %p\n", vec,rt_current);
	        rt_task_suspend(rt_current);
        	rt_task_delete(rt_current); // In case the suspend does not work ?
        	return 1;
	}
}
//	rt_printk("Default Trap Handler: vector %d: task %X\n", vec, current);
	if (vec == 7) {
		do {
			unsigned long flags;
			hard_save_flags_and_cli(flags);
			if (!test_and_set_bit(IFLAG, &flags)) {
				__cli();
			}
			hard_restore_flags(flags);
		} while (0);

		if (current->used_math) {
			restore_fpu(current);
		} else {
			init_fpu(current);
		}
		htrp7++;
		return 1;
	}
	if ((rt_task = current->this_rt_task[0]) && rt_task->is_hard > 0) {
        	give_back_to_linux(rt_task);
		rt_task->is_hard = 2;
	}
#endif /* CONFIG_RTAI_ADEOS */
	return 0;
}

void start_buddy(RT_TASK *rt_task) { }
void end_buddy(RT_TASK *rt_task)   { }
 
extern int (*rtai_signal_handler)(struct task_struct *lnxt, int sig);

static int linux_signal_handler(struct task_struct *lnxtsk, int sig)
{
        RT_TASK *task = (RT_TASK *)lnxtsk->this_rt_task[0];
        if ((task->force_soft = task->is_hard > 0)) {
                rt_global_cli();
                if (task->state != READY) {
                	task->state &= ~READY;
                        enq_ready_task(task);
                	RT_SCHEDULE(task, hard_cpu_id());
                }
                rt_global_sti();
                return 0;
        }
        if (task->state) {
                lnxtsk->state = TASK_INTERRUPTIBLE;
	}
        return 1;
}

int use_buddy_version;

static int usp_init_module(void)
{
	int cpuid;

	linux_syscall_handler = rt_set_intr_handler(SYSCALL_VECTOR, (void *)LXRT_LINUX_SYSCALL_TRAP);
       	rthal.lxrt_global_cli = linux_lxrt_global_cli;
	sched_trap_handler = rt_set_rtai_trap_handler(rtai_trap_handler);
	rtai_signal_handler = linux_signal_handler;
//                                            2865600023UL is nam2num("USPAPP")
	if ((wake_up_srq.srq = rt_request_srq(2865600023UL, wake_up_srq_handler, 0)) < 0) {
		printk("NEWLXRT: no wake_up_srq available.\n");
		return wake_up_srq.srq;
	}
	if (Reservoir <= 0) {
		Reservoir = 1;
	}
	Reservoir = (Reservoir + NR_RT_CPUS - 1)/NR_RT_CPUS;
	for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		taskav[cpuid] = (void *)kmalloc(Reservoir*sizeof(void *), GFP_KERNEL);
		init_MUTEX_LOCKED(&resem[cpuid]);
		kernel_thread((void *)kthread_b, (void *)cpuid, 0);
		kernel_thread((void *)kthread_m, (void *)cpuid, 0);
		down(&resem[cpuid]);
		down(&resem[cpuid]);
		klistm[cpuid].in = (2*Reservoir) & (MAX_SRQ - 1);
		wake_up_process(kthreadm[cpuid]);
	}
	return 0 ;
}

static void usp_cleanup_module(void)
{
	int cpuid;

	for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		struct klist_t *klistp;
		struct task_struct *kthread;
		klistp = &klistm[cpuid];
		while ((kthread = __get_kthread(cpuid))) {
			klistp->task[klistp->in] = kthread;
			klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
		}
		wake_up_process(kthreadm[cpuid]);
		while (kthreadm[cpuid]->state != TASK_UNINTERRUPTIBLE) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
	}
	endkthread = 1;
	for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		wake_up_process(kthreadb[cpuid]);
		wake_up_process(kthreadm[cpuid]);
		while (kthreadb[cpuid] || kthreadm[cpuid]) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
		kfree(taskav[cpuid]);
	}
	rtai_signal_handler = 0;
	rt_set_rtai_trap_handler(sched_trap_handler);
       	rthal.lxrt_global_cli = 0;
	if (rt_free_srq(wake_up_srq.srq) < 0) {
		printk("NEWLXRT: wake_up_srq %d illegal or already free.\n", wake_up_srq.srq);
	}

	rt_reset_intr_handler(SYSCALL_VECTOR, linux_syscall_handler);
	if (htrp7) {
		printk("\nNEWLXRT HAD TO MANAGE %d UNEXPECTED TRAP7.\n\n", htrp7);
	}
	return;
}

/* ++++++++++++++++++++ INTERNAL COND VARIABLES SUPPORT +++++++++++++++++++++ */

int rt_cond_wait(CND *cnd, SEM *mtx)
{
	rt_sem_signal(mtx);
	rt_sem_wait(cnd);
	rt_sem_wait(mtx);
	return 0;
}

int rt_cond_wait_until(CND *cnd, SEM *mtx, RTIME time)
{
	int cndret;

	rt_sem_signal(mtx);
	cndret = 0;
	if (rt_sem_wait_until(cnd, time) >= SEM_TIMOUT) {
		return -1;
	}
	return rt_sem_wait_until(mtx, time) >= SEM_TIMOUT ? -1 : 0;
}

int rt_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay)
{
	return rt_cond_wait_until(cnd, mtx, get_time() + delay);
}

/* +++++++++++++++++++++++++ READERS-WRITER LOCKS +++++++++++++++++++++++++++ */

int rt_rwl_init(RWL *rwl)
{
	rt_typed_sem_init(&rwl->wrmtx, 1, RES_SEM);
	rt_typed_sem_init(&rwl->wrsem, 0, CNT_SEM);
	rt_typed_sem_init(&rwl->rdsem, 0, CNT_SEM);
	return 0;
}

int rt_rwl_delete(RWL *rwl)
{
	int ret;

	ret  =  rt_sem_delete(&rwl->rdsem);
	ret |= !rt_sem_delete(&rwl->wrsem);
	ret |= !rt_sem_delete(&rwl->wrmtx);
	return ret ? 0 : SEM_ERR;
}

int rt_rwl_rdlock(RWL *rwl)
{
	unsigned long flags;
	RT_TASK *wtask;
	DECLARE_RT_CURRENT;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	while (rwl->wrmtx.owndby || ((wtask = (rwl->wrsem.queue.next)->task) && wtask->priority <= rt_current->priority)) {
		int ret;
		if (rwl->wrmtx.owndby == rt_current) {
			rt_global_restore_flags(flags);
			return SEM_ERR + 1;
		}
		if ((ret = rt_sem_wait(&rwl->rdsem)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		}
	}
	((int *)&rwl->rdsem.owndby)[0]++;
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_rdlock_if(RWL *rwl)
{
	unsigned long flags;
	RT_TASK *wtask;

	flags = rt_global_save_flags_and_cli();
	if (!rwl->wrmtx.owndby && (!(wtask = (rwl->wrsem.queue.next)->task) || wtask->priority > RT_CURRENT->priority)) {
		((int *)&rwl->rdsem.owndby)[0]++;
		rt_global_restore_flags(flags);
		return 0;
	}
	rt_global_restore_flags(flags);
	return -1;
}

int rt_rwl_rdlock_until(RWL *rwl, RTIME time)
{
	unsigned long flags;
	RT_TASK *wtask;
	DECLARE_RT_CURRENT;

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	while (rwl->wrmtx.owndby || ((wtask = (rwl->wrsem.queue.next)->task) && wtask->priority <= rt_current->priority)) {
		int ret;
		if (rwl->wrmtx.owndby == rt_current) {
			rt_global_restore_flags(flags);
			return SEM_ERR + 1;
		}
		if ((ret = rt_sem_wait_until(&rwl->rdsem, time)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		}
	}
	((int *)&rwl->rdsem.owndby)[0]++;
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_rdlock_timed(RWL *rwl, RTIME delay)
{
	return rt_rwl_rdlock_until(rwl, get_time() + delay);
}

int rt_rwl_wrlock(RWL *rwl)
{
	unsigned long flags;
	int ret;

	flags = rt_global_save_flags_and_cli();
	while (rwl->rdsem.owndby) {
		if ((ret = rt_sem_wait(&rwl->wrsem)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		}
	}
	if ((ret = rt_sem_wait(&rwl->wrmtx)) >= SEM_TIMOUT) {
		rt_global_restore_flags(flags);
		return ret;
	}
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_wrlock_if(RWL *rwl)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (!rwl->rdsem.owndby && rt_sem_wait_if(&rwl->wrmtx) >= 0) {
		rt_global_restore_flags(flags);
		return 0;
	}
	rt_global_restore_flags(flags);
	return -1;
}

int rt_rwl_wrlock_until(RWL *rwl, RTIME time)
{
	unsigned long flags;
	int ret;

	flags = rt_global_save_flags_and_cli();
	while (rwl->rdsem.owndby) {
		if ((ret = rt_sem_wait_until(&rwl->wrsem, time)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		};
	}
	if ((ret = rt_sem_wait_until(&rwl->wrmtx, time)) >= SEM_TIMOUT) {
		rt_global_restore_flags(flags);
		return ret;
	};
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_wrlock_timed(RWL *rwl, RTIME delay)
{
	return rt_rwl_wrlock_until(rwl, get_time() + delay);
}

int rt_rwl_unlock(RWL *rwl)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (rwl->wrmtx.owndby) {
		rt_sem_signal(&rwl->wrmtx);
	} else if (rwl->rdsem.owndby) {
    		((int)rwl->rdsem.owndby)--;
	}
	rt_global_restore_flags(flags);
	flags = rt_global_save_flags_and_cli();
	if (!rwl->wrmtx.owndby && !rwl->rdsem.owndby) {
		RT_TASK *wtask, *rtask;
		wtask = (rwl->wrsem.queue.next)->task;
		rtask = (rwl->rdsem.queue.next)->task;
		if (wtask && rtask) {
			if (wtask->priority < rtask->priority) {
				rt_sem_signal(&rwl->wrsem);
			} else {
				rt_sem_signal(&rwl->rdsem);
			}
		} else if (wtask) {
			rt_sem_signal(&rwl->wrsem);
		} else if (rtask) {
			rt_sem_signal(&rwl->rdsem);
		}
        }
	rt_global_restore_flags(flags);
	return 0;
}

/* +++++++++++++++++++++ RECURSIVE SPINLOCKS SUPPORT ++++++++++++++++++++++++ */

int rt_spl_init(SPL *spl)
{
	spl->owndby = 0;
	spl->count  = 0;
	return 0;
}

int rt_spl_delete(SPL *spl)
{
        return 0;
}

int rt_spl_lock(SPL *spl)
{
	unsigned long flags;
	DECLARE_RT_CURRENT;

	hard_save_flags_and_cli(flags);
	ASSIGN_RT_CURRENT;
	if (spl->owndby == rt_current) {
		spl->count++;
	} else {
		while (cmpxchg(&spl->owndby, 0, rt_current));
		spl->flags = flags;
	}
	return 0;
}

int rt_spl_lock_if(SPL *spl)
{
	unsigned long flags;
	DECLARE_RT_CURRENT;

	hard_save_flags_and_cli(flags);
	ASSIGN_RT_CURRENT;
	if (spl->owndby == rt_current) {
		spl->count++;
	} else {
		if (cmpxchg(&spl->owndby, 0, rt_current)) {
			hard_restore_flags(flags);
			return -1;
		}
		spl->flags = flags;
	}
	return 0;
}

int rt_spl_lock_timed(SPL *spl, unsigned long ns)
{
	unsigned long flags;
	DECLARE_RT_CURRENT;

	hard_save_flags_and_cli(flags);
	ASSIGN_RT_CURRENT;
	if (spl->owndby == rt_current) {
		spl->count++;
	} else {
		RTIME end_time;
		void *locked;
		end_time = rdtsc() + imuldiv(ns, tuned.cpu_freq, 1000000000);
		while ((locked = cmpxchg(&spl->owndby, 0, rt_current)) && rdtsc() < end_time);
		if (locked) {
			hard_restore_flags(flags);
			return -1;
		}
		spl->flags = flags;
	}
	return 0;
}

int rt_spl_unlock(SPL *spl)
{
	unsigned long flags;
	DECLARE_RT_CURRENT;

	hard_save_flags_and_cli(flags);
	ASSIGN_RT_CURRENT;
	if (spl->owndby == rt_current) {
		if (spl->count) {
			--spl->count;
		} else {
			spl->owndby = 0;
			spl->count  = 0;
			hard_restore_flags(spl->flags);
		}
		return 0;
	}
	hard_restore_flags(flags);
	return -1;
}

/* +++++++++++++++++++++++++++++_END_END_END_++++++++++++++++++++++++++++++++ */
