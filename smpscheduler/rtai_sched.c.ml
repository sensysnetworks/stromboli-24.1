/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

/*
ACKNOWLEDGMENTS: 
- Steve Papacharalambous (stevep@zentropix.com) has contributed a very 
  informative proc filesystem procedure.
- Stefano Picerno (stefanopp@libero.it) for suggesting a simple fix to
  distinguish a timeout from an abnormal retrun in timed sem waits.
- Geoffrey Martin (gmartin@altersys.com) for a fix to functions with timeouts.
- Michael D. Kralka (michael.kralka@kvs.com) for a fix to rt_change_priority.
*/


#define ALLOW_RR

#define ONE_SHOT 	0
#define PREEMPT_ALWAYS	0
#define LINUX_FPU 	1

#define SCHED_IPI     RTAI_3_IPI
#define SCHED_VECTOR  RTAI_3_VECTOR

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/timex.h>
#include <linux/sched.h>

#include <asm/param.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif

#define INTERFACE_TO_LINUX
#include <rtai.h>
#include <asm/rtai_sched.h>
#include <rtai_sched.h>

MODULE_LICENSE("GPL");


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
#define set_exit_handler(task, fun, arg1, arg2) __set_exit_handler(task, fun, arg1, arg2)
#else
#define sched_malloc(size)		kmalloc((size), GFP_KERNEL)
#define sched_free(adr)			kfree((adr))
#define sched_mem_init()
#define sched_mem_end()
#define call_exit_handlers(task) 
#define set_exit_handler(task, fun, arg1, arg2)
#endif

#define RT_SEM_MAGIC 0xaabcdeff

#define SEM_ERR (0xFfff)

#define MSG_ERR ((RT_TASK *)0xFfff)

#define NOTHING   ((void *)0)

#define SOMETHING ((void *)1)

#ifdef __USE_APIC__

#define TIMER_CHIP "APIC"
#define TIMER_FREQ FREQ_APIC
#define TIMER_LATENCY LATENCY_APIC
#define TIMER_SETUP_TIME SETUP_TIME_APIC
static volatile int timer_cpu;
#define set_timer_cpu(cpu_map)  do { timer_cpu = cpu_map; } while (0)
#define is_timer_cpu(cpuid) \
	if (!test_bit((cpuid), &timer_cpu)) { \
		sched_release_global_lock((cpuid)); \
		return; \
	}
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
int rt_get_timer_cpu(void) { return timer_cpu; }
#define update_linux_timer()
extern void broadcast_to_local_timers(int irq,void *dev_id,struct pt_regs *regs);
#define BROADCAST_TO_LOCAL_TIMERS() broadcast_to_local_timers(-1,NULL,NULL)
#define FREE_LOCAL_TIMERS() rt_free_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers)

#else

#define TIMER_CHIP "8254"
#define TIMER_FREQ FREQ_8254
#define TIMER_LATENCY LATENCY_8254
#define TIMER_SETUP_TIME SETUP_TIME_8254
#define set_timer_cpu(cpu_map) 
#define is_timer_cpu(cpuid)
#define set_timer_chip(delay) \
	{ \
		/*rt_spin_lock(&timer_chip_lock);*/ \
		rt_set_timer_delay(delay); \
		/*rt_spin_unlock(&timer_chip_lock);*/ \
	}
//static spinlock_t timer_chip_lock = SPIN_LOCK_UNLOCKED;
int rt_get_timer_cpu(void) { return -EINVAL; }
#define update_linux_timer() rt_pend_linux_irq(TIMER_8254_IRQ)
#define BROADCAST_TO_LOCAL_TIMERS()
#define FREE_LOCAL_TIMERS()

#endif

#ifdef CONFIG_PROC_FS
// proc filesystem additions.
static int rtai_proc_sched_register(void);
static void rtai_proc_sched_unregister(void);
// End of proc filesystem additions. 
#endif

static RT_TASK *rt_smp_current[NR_RT_CPUS];

static int sched_rqsted[NR_RT_CPUS];

DEFINE_LINUX_SMP_CR0;

static RT_TASK rt_smp_linux_task[NR_RT_CPUS];

#define DECLARE_RT_CURRENT RT_TASK *rt_current

#define RT_CURRENT rt_smp_current[hard_cpu_id()]

#define ASSIGN_RT_CURRENT rt_current = rt_smp_current[hard_cpu_id()]

#define linux_cr0 (linux_smp_cr0[cpuid])

#define rt_linux_task (rt_smp_linux_task[cpuid])

#define rt_base_linux_task (rt_smp_linux_task[0/* it was NR_RT_CPUS - 1*/])

static RTIME rt_time_h;

static int rt_half_tick;

static int oneshot_timer;

static int oneshot_running;

static int shot_fired;

static int preempt_always;

static rwlock_t task_list_lock = RW_LOCK_UNLOCKED;

static RT_TASK *wdog_task[NR_RT_CPUS];

#define SEMHLF 0x0000FFFF
#define RPCHLF 0xFFFF0000
#define RPCINC 0x00010000

#define MAX_SRQ  64
static struct { int srq, in, out; void *mp[MAX_SRQ]; } frstk_srq;

/* ++++++++++++++++++++++++++++++++ TASKS ++++++++++++++++++++++++++++++++++ */

static inline void enq_ready_edf_task(RT_TASK *ready_task)
{
	RT_TASK *task;
	task = rt_base_linux_task.rnext;
	while (task->policy < 0 && ready_task->period >= task->period) {
		task = task->rnext;
	}
	task->rprev = (ready_task->rprev = task->rprev)->rnext = ready_task;
	ready_task->rnext = task;
}

static inline void enq_ready_task(RT_TASK *ready_task)
{
	RT_TASK *task;
	task = rt_base_linux_task.rnext;
	while (ready_task->priority >= task->priority) {
		if ((task = task->rnext)->priority < 0) break;
	}
	task->rprev = (ready_task->rprev = task->rprev)->rnext = ready_task;
	ready_task->rnext = task;
}

static inline int renq_ready_task(RT_TASK *ready_task, int priority)
{
	int retval;
	if ((retval = ready_task->priority != priority)) {
		ready_task->priority = priority;
		(ready_task->rprev)->rnext = ready_task->rnext;
		(ready_task->rnext)->rprev = ready_task->rprev;
		enq_ready_task(ready_task);
	}
	return retval;
}

static inline void rem_ready_task(RT_TASK *task)
{
	if (task->state && !(task->state & ~(READY | RUNNING))) {
		(task->rprev)->rnext = task->rnext;
		(task->rnext)->rprev = task->rprev;
	}
}

static inline void rem_ready_current(RT_TASK *rt_current)
{
	(rt_current->rprev)->rnext = rt_current->rnext;
	(rt_current->rnext)->rprev = rt_current->rprev;
}

#define TASK_TO_SCHEDULE_ON_IPI() \
	while ((task = task->rnext) != &rt_base_linux_task) { \
		if (task->state == READY && test_bit(cpuid, &(task->runnable_on_cpus))) { \
			prio = (new_task = task)->priority; \
			break; \
		} \
	}

#define TASK_TO_SCHEDULE() \
	while ((task = task->rnext) != &rt_base_linux_task) { \
		if (task->state == READY ) { \
			if (test_bit(cpuid, &(task->runnable_on_cpus))) { \
				prio = (new_task = task)->priority; \
				while ((task = task->rnext) != &rt_base_linux_task) { \
					if (task->state == READY ) { \
						cpus_with_ready_tasks |= task->runnable_on_cpus; \
					} \
				} \
				break; \
			} else { \
				cpus_with_ready_tasks |= task->runnable_on_cpus; \
			} \
		} \
	}

static inline void enq_timed_task(RT_TASK *timed_task)
{
	RT_TASK *task;
	task = rt_base_linux_task.tnext;
	while (timed_task->resume_time > task->resume_time) {
		task = task->tnext;
	}
	task->tprev = (timed_task->tprev = task->tprev)->tnext = timed_task;
	timed_task->tnext = task;
}

static inline void wake_up_timed_tasks(void)
{
	RT_TASK *task;
	task = rt_base_linux_task.tnext;
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
	rt_base_linux_task.tnext = task;
	task->tprev = &rt_base_linux_task;
}

static inline void rem_timed_task(RT_TASK *task)
{
	if ((task->state & DELAYED)) {
		(task->tprev)->tnext = task->tnext;
		(task->tnext)->tprev = task->tprev;
	}
}

static void rt_startup(void(*rt_thread)(int), int data)
{
	extern int rt_task_delete(RT_TASK *);
	rt_global_sti();
	RT_CURRENT->exectime[1] = rdtsc();
	rt_thread(data);
	rt_task_delete(rt_smp_current[hard_cpu_id()]);
}

#ifdef CONFIG_SMP

#define cpu_present_map cpu_online_map

static inline void sched_get_global_lock(int cpuid)
{
	if (!test_and_set_bit(cpuid, locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus)) {
#ifdef STAGGER
			STAGGER(cpuid);
#endif
		}
	}
}


static inline void sched_release_global_lock(int cpuid)
{
	if (test_and_clear_bit(cpuid, locked_cpus)) {
		test_and_clear_bit(31, locked_cpus);
#ifdef STAGGER
		STAGGER(cpuid);
#endif
	}
}


static inline void send_sched_ipi(unsigned long dest)
{
	if (dest) {
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
}

#else

static unsigned long cpu_present_map = 1;

#define sched_get_global_lock(cpuid)

#define sched_release_global_lock(cpuid)

#define send_sched_ipi(dest)

#endif


int rt_task_init(RT_TASK *task, void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu,
			void(*signal)(void))
{
	unsigned long flags;
	int cpuid, *st, i, ok;

	if (task->magic == RT_TASK_MAGIC || priority < 0) {
		return -EINVAL;
	}
	if (!(st = (int *)sched_malloc(stack_size))) {   
		return -ENOMEM;
	}
	if (priority == RT_HIGHEST_PRIORITY) {	    // watchdog reserves highest
	    for (i = ok = 0; i < NR_RT_CPUS; i++) { // priority task on each CPU
		if (!wdog_task[i] || wdog_task[i] == task) {
		    ok = 1;
		    break;
		}
	    }
	    if (!ok) {
		rt_printk("Highest priority reserved for RTAI watchdog\n");
		return -EBUSY;
	    }
	}

	task->bstack = task->stack = (int *)(((unsigned long)st + stack_size - 0x10) & ~0xF);
	task->stack[0] = 0;
	task->uses_fpu = uses_fpu ? 1 : 0;
	task->runnable_on_cpus = cpu_present_map;
	*(task->stack_bottom = st) = 0;
	task->lnxtsk = 0;
	task->magic = RT_TASK_MAGIC; 
	task->policy = 0;
	task->suspdepth = 1;
	task->state = (SUSPENDED | READY);
	task->owndres = 0;
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
	task->tick_queue        = NOTHING;
	task->trap_handler_data = NOTHING;
	task->resync_frame = 0;
	task->ExitHook = 0;
	task->exectime[0] = 0;
	task->system_data_ptr = 0;

	init_arch_stack();

	flags = rt_global_save_flags_and_cli();
	cpuid = hard_cpu_id();
#define fpu_task (&rt_linux_task)  // needed just by the very next line of code
	init_fp_env();
	task->next = 0;
	read_lock(&task_list_lock);
	rt_base_linux_task.prev->next = task;
	task->prev = rt_base_linux_task.prev;
	rt_base_linux_task.prev = task;
	read_unlock(&task_list_lock);
	rt_global_restore_flags(flags);

	return 0;
}


int rt_task_init_cpuid(RT_TASK *task, void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu,
			void(*signal)(void), unsigned int cpuid)
{
	int retval;

	retval = rt_task_init(task, rt_thread, data, stack_size, priority, 
							uses_fpu, signal);
	rt_set_runnable_on_cpus(task, 1 << cpuid);
	return retval;
}


void rt_set_runnable_on_cpus(RT_TASK *task, unsigned int runnable_on_cpus)
{
	unsigned long flags;
	flags = rt_global_save_flags_and_cli();
	if (!(task->runnable_on_cpus = (runnable_on_cpus & cpu_present_map))) {
		task->runnable_on_cpus = 1;
	}
	rt_global_restore_flags(flags);
}


void rt_set_runnable_on_cpuid(RT_TASK *task, unsigned int cpuid)
{
	rt_set_runnable_on_cpus(task, 1 << cpuid);
}


int rt_check_current_stack(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags, cpuid;
	char *sp;
 
	hard_save_flags_and_cli(flags);
	if ((rt_current = rt_smp_current[cpuid = hard_cpu_id()]) != &rt_linux_task) {
		sp = get_stack_pointer();
		hard_restore_flags(flags);
		return (sp - (char *)(rt_current->stack_bottom));
	} else {
		hard_restore_flags(flags);
		return -0x7FFFFFFF;
	}
}


void rt_set_sched_policy(RT_TASK *task, int policy, int rr_quantum_ns)
{
	if ((task->policy = policy ? 1 : 0)) {
		task->rr_quantum = nano2count(rr_quantum_ns);
		if ((task->rr_quantum & 0xF0000000) || !task->rr_quantum) {
			task->rr_quantum = rt_times.linux_tick;
		}
		task->rr_remaining = task->rr_quantum;
		task->yield_time = 0;
	}
}


#ifdef ALLOW_RR
static RTIME global_yield_time = RT_TIME_END;
static int global_policy;

#define RR_YIELD() \
if (rt_current->policy > 0) { \
	rt_current->rr_remaining = rt_current->yield_time - rt_times.tick_time; \
	if (rt_current->rr_remaining <= 0) { \
		rt_current->rr_remaining = rt_current->rr_quantum; \
		if (rt_current->state == READY || rt_current->state == (READY | RUNNING)) { \
			RT_TASK *task; \
			task = rt_current->rnext; \
			while (rt_current->priority == task->priority) { \
				task = task->rnext; \
			} \
			if (task != rt_current->rnext) { \
				rt_current->state &= ~RUNNING; \
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
	global_policy = 1; \
	if (new_task->yield_time < global_yield_time) { \
		global_yield_time = new_task->yield_time; \
	} \
}

#define RR_SPREMP() \
if (global_policy > 0) { \
	preempt = 1; \
	if (global_yield_time < intr_time) { \
		RTIME t; \
		t = intr_time; \
		intr_time = global_yield_time; \
		global_yield_time = t; \
	} else { \
		global_yield_time = intr_time; \
	} \
	global_policy = 0; \
} else { \
	preempt = 0; \
}

#define RR_TPREMP() \
if (global_policy > 0) { \
	preempt = 1; \
	if (global_yield_time < rt_times.intr_time) { \
		RTIME t; \
		t = rt_times.intr_time; \
		rt_times.intr_time = global_yield_time; \
		global_yield_time = t; \
	} else { \
		global_yield_time = rt_times.intr_time; \
	} \
	global_policy = 0; \
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
#else
#define KEXECTIME()
#endif

//#define CAUTIOUS
#ifdef CAUTIOUS

static void rt_schedule_on_schedule_ipi(void)
{
	DECLARE_RT_CURRENT;
	RT_TASK *task, *new_task;
	int prio, cpuid;

	prio = RT_LINUX_PRIORITY;
	rt_current = rt_smp_current[cpuid = hard_cpu_id()];
	sched_rqsted[cpuid] = 1;
	new_task = &rt_linux_task;
	task = &rt_base_linux_task;

	sched_get_global_lock(cpuid);
	rt_current->state &= ~RUNNING;
	RR_YIELD();
        TASK_TO_SCHEDULE_ON_IPI();
	RR_SETYT();
	new_task->state = (READY | RUNNING);

	if (new_task != rt_current) {
		if (rt_current == &rt_linux_task) {
			rt_switch_to_real_time(cpuid);
			save_cr0_and_clts(linux_cr0);
		}
		if (rt_current->uses_fpu) {
			enable_fpu();
			save_fpenv(rt_current->fpu_reg);
			if (new_task->uses_fpu) {
				restore_fpenv(new_task->fpu_reg);
			}
		} else if (new_task->uses_fpu) {
			enable_fpu();
			restore_fpenv(new_task->fpu_reg);
		}
		KEXECTIME();
		rt_exchange_tasks(rt_smp_current[cpuid], new_task);
		sched_release_global_lock(hard_cpu_id());
		if (rt_current->signal) {
			(*rt_current->signal)();
		}
		return;
	} else {
		sched_release_global_lock(cpuid);
	}
	return;
}

#else

static void rt_schedule_on_schedule_ipi(void)
{
	DECLARE_RT_CURRENT;
	RT_TASK *task, *new_task;
	int prio, cpuid;

	rt_current = rt_smp_current[cpuid = hard_cpu_id()];
	sched_rqsted[cpuid] = 1;
	do {
		prio = RT_LINUX_PRIORITY;
		new_task = &rt_linux_task;
		task = &rt_base_linux_task;

		read_lock(&task_list_lock);
		RR_YIELD();
		TASK_TO_SCHEDULE_ON_IPI();
		RR_SETYT();
		read_unlock(&task_list_lock);

		if (rt_current->state == (READY | RUNNING) && rt_current->priority <= prio) {
			return;
		}
		sched_get_global_lock(cpuid);
		if (new_task->state == READY) {
			rt_current->state &= ~RUNNING;
			new_task->state = (READY | RUNNING);
			if (rt_current == &rt_linux_task) {
				rt_switch_to_real_time(cpuid);
				save_cr0_and_clts(linux_cr0);
			}
			if (rt_current->uses_fpu) {
				enable_fpu();
				save_fpenv(rt_current->fpu_reg);
				if (new_task->uses_fpu) {
					restore_fpenv(new_task->fpu_reg);
				}
			} else if (new_task->uses_fpu) {
				enable_fpu();
				restore_fpenv(new_task->fpu_reg);
			}
			KEXECTIME();
			rt_exchange_tasks(rt_smp_current[cpuid], new_task);
			sched_release_global_lock(hard_cpu_id());
			if (rt_current->signal) {
				(*rt_current->signal)();
			}
			return;
		}
		sched_release_global_lock(cpuid);
	} while (1);
}

#endif

#define ANTICIPATE

static void rt_schedule(void)
{
	DECLARE_RT_CURRENT;
	RT_TASK *task, *new_task;
	RTIME intr_time, now;
	unsigned int cpus_with_ready_tasks;
	int prio, delay, preempt, cpuid;

	task = &rt_base_linux_task;
	cpus_with_ready_tasks = 0;
	prio = RT_LINUX_PRIORITY;
	rt_current = rt_smp_current[cpuid = hard_cpu_id()];
	new_task = &rt_linux_task;
	rt_current->state &= ~RUNNING;
	RR_YIELD();
	if (oneshot_running) {
#ifdef ANTICIPATE
		rt_time_h = rdtsc() + rt_half_tick;
		wake_up_timed_tasks();
#endif
                TASK_TO_SCHEDULE();
		RR_SETYT();

		intr_time = shot_fired ? rt_times.intr_time :
			    rt_times.intr_time + rt_times.linux_tick;
		RR_SPREMP();
		task = &rt_base_linux_task;
                while ((task = task->tnext) != &rt_base_linux_task) {
                        if (task->priority <= prio && task->resume_time < intr_time) {
                                intr_time = task->resume_time;
                                preempt = 1;
                                break;
                        }
                }

		if (preempt || (!shot_fired && (prio == RT_LINUX_PRIORITY))) {
			shot_fired = 1;
			if (preempt) {
				rt_times.intr_time = intr_time;
			}
			delay = (int)(rt_times.intr_time - (now = rdtsc())) - tuned.latency;
			if (delay >= tuned.setup_time_TIMER_CPUNIT) {
				delay = imuldiv(delay, TIMER_FREQ, tuned.cpu_freq);
			} else {
				delay = tuned.setup_time_TIMER_UNIT;
				rt_times.intr_time = now + (tuned.setup_time_TIMER_CPUNIT);
			}
			set_timer_cpu(1 << cpuid);
			set_timer_chip(delay);
		}
	} else {
                TASK_TO_SCHEDULE();
		RR_SETYT();
	}

	new_task->state = (READY | RUNNING);
	send_sched_ipi(cpus_with_ready_tasks & ~(1 << cpuid));

	if (new_task != rt_current) {
		if (rt_current == &rt_linux_task) {
			rt_switch_to_real_time(cpuid);
			save_cr0_and_clts(linux_cr0);
		}
		if (rt_current->uses_fpu) {
			enable_fpu();
			save_fpenv(rt_current->fpu_reg);
			if (new_task->uses_fpu) {
				restore_fpenv(new_task->fpu_reg);
			}
		} else if (new_task->uses_fpu) {
			enable_fpu();
			restore_fpenv(new_task->fpu_reg);
		}
		if (new_task == &rt_linux_task) {
			rt_switch_to_linux(cpuid);
			restore_cr0(linux_cr0);
		}
		KEXECTIME();
		rt_exchange_tasks(rt_smp_current[cpuid], new_task);
		if (rt_current->signal) {
			sched_release_global_lock(cpuid = hard_cpu_id());
			(*rt_current->signal)();
			sched_get_global_lock(cpuid);
		}
	}
/*
	 else {
		sched_release_global_lock(cpuid);
		sched_get_global_lock(cpuid);
	}
*/
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
	prio = 0;
	task = &rt_base_linux_task;
	while ((task = task->next)) {
		RT_TASK *task, *htask;
		RTIME period;
		htask = 0;
		task = &rt_base_linux_task;
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
ret:	task = &rt_base_linux_task;
	while ((task = task->next)) {
		if (task->priority < 0) {
			task->priority = task->base_priority;
		}
	}
	return;
}


int rt_change_prio(RT_TASK *task, int priority)
{
	unsigned long flags;
	int prio, sched;

	if (task->magic != RT_TASK_MAGIC || priority < 0) {
		return -EINVAL;
	}

	sched = 0;
	if ((prio = task->base_priority) == priority) {
		return prio;
	}
	flags = rt_global_save_flags_and_cli();
	if ((task->base_priority = priority) < task->priority || prio == task->priority) {
		QUEUE *q;
		do {
			task->priority = priority;
			if (task->state && !(task->state & ~(READY | RUNNING))) {
				(task->rprev)->rnext = task->rnext;
				(task->rnext)->rprev = task->rprev;
				enq_ready_task(task);
				sched = 1;
			} else if ((q = task->blocked_on) && !((task->state & SEMAPHORE) && ((SEM *)q)->qtype)) {
				(task->queue.prev)->next = task->queue.next;
				(task->queue.next)->prev = task->queue.prev;
				while ((q = q->next) != task->blocked_on && (q->task)->priority <= priority);
				q->prev = (task->queue.prev = q->prev)->next  = &(task->queue);
				task->queue.next = q;
                                sched = 1;
			}
		} while ((task = task->prio_passed_to) && task->priority > priority);
		if (sched) {
			rt_schedule();
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
		sched_rqsted[hard_cpu_id()] = rt_current->priority = -1;
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
		if (sched_rqsted[hard_cpu_id()] > 0) {
			rt_schedule();
		}
	}
	rt_global_restore_flags(flags);
}


int rt_task_delete(RT_TASK *task)
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
			if (((q->task)->state & ~(READY | RUNNING)) && ((q->task)->state &= ~(SEND | RPC | DELAYED)) == READY) {
				enq_ready_task(q->task);
			}       
			(q->task)->blocked_on = 0;
		}       
                q = &(task->ret_queue);
       	        while ((q = q->next) != &(task->ret_queue)) {
			rem_timed_task(q->task);
               	        if (((q->task)->state & ~(READY | RUNNING)) && ((q->task)->state &= ~(RETURN | DELAYED)) == READY) {
				enq_ready_task(q->task);
			}       
			(q->task)->blocked_on = 0;
                }
		write_lock(&task_list_lock);
		if (!((task->prev)->next = task->next)) {
			rt_base_linux_task.prev = task->prev;
		} else {
			(task->next)->prev = task->prev;
		}
		write_unlock(&task_list_lock);
		frstk_srq.mp[frstk_srq.in] = task->stack_bottom;
		frstk_srq.in = (frstk_srq.in + 1) & (MAX_SRQ - 1);
		task->magic = 0;
		rt_pend_linux_srq(frstk_srq.srq);
		rem_ready_task(task);
		task->state = 0;
		if (task == rt_current) {
			rt_schedule();
		}
	} else {
		task->suspdepth = -0x7FFFFFFF;
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_get_task_state(RT_TASK *task)
{
	return task->state;
}


static void rt_timer_handler(void)
{
	DECLARE_RT_CURRENT;
	RTIME now;
	RT_TASK *task, *new_task;
	unsigned int cpus_with_ready_tasks;
	int prio, delay, preempt, cpuid; 

	rt_current = rt_smp_current[cpuid = hard_cpu_id()];
	sched_rqsted[cpuid] = 1;
	task = &rt_base_linux_task;
	prio = RT_LINUX_PRIORITY;
	cpus_with_ready_tasks = 0;
	new_task = &rt_linux_task;

	sched_get_global_lock(cpuid);
	is_timer_cpu(cpuid);
#ifdef CONFIG_X86_REMOTE_DEBUG
	if (oneshot_timer) {	// Resync after possibly hitting a breakpoint
	    	rt_times.intr_time = rdtsc();
	}
#endif
	rt_times.tick_time = rt_times.intr_time;
	rt_time_h = rt_times.tick_time + rt_half_tick;
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		update_linux_timer();
	}
	rt_current->state &= ~RUNNING;
	wake_up_timed_tasks();
	RR_YIELD();
	TASK_TO_SCHEDULE();
	RR_SETYT();

	if (oneshot_timer) {
		rt_times.intr_time = rt_times.linux_time > rt_times.tick_time ?
		rt_times.linux_time : rt_times.tick_time + rt_times.linux_tick;
		RR_TPREMP();

		task = &rt_base_linux_task;
                while ((task = task->tnext) != &rt_base_linux_task) {
                        if (task->priority <= prio && task->resume_time < rt_times.intr_time) {
                                rt_times.intr_time = task->resume_time;
                                preempt = 1;
                                break;
                        }
                }
		if ((shot_fired = preempt)) {
			delay = (int)(rt_times.intr_time - (now = rdtsc())) - tuned.latency;
			if (delay >= tuned.setup_time_TIMER_CPUNIT) {
				delay = imuldiv(delay, TIMER_FREQ, tuned.cpu_freq);
			} else {
				delay = tuned.setup_time_TIMER_UNIT;
				rt_times.intr_time = now + (tuned.setup_time_TIMER_CPUNIT);
			}
			set_timer_cpu(1 << cpuid);
			set_timer_chip(delay);
		}
	} else {
		rt_times.intr_time += rt_times.periodic_tick;
		rt_set_timer_delay(0);
	}
	new_task->state = (READY | RUNNING);
	send_sched_ipi(cpus_with_ready_tasks & ~(1 << cpuid));

	if (new_task != rt_current) {
		if (rt_current == &rt_linux_task) {
			rt_switch_to_real_time(cpuid);
			save_cr0_and_clts(linux_cr0);
		}
		if (rt_current->uses_fpu) {
			enable_fpu();
			save_fpenv(rt_current->fpu_reg);
			if (new_task->uses_fpu) {
				restore_fpenv(new_task->fpu_reg);
			}
		} else if (new_task->uses_fpu) {
			enable_fpu();
			restore_fpenv(new_task->fpu_reg);
		}
		KEXECTIME();
		rt_exchange_tasks(rt_smp_current[cpuid], new_task);
		sched_release_global_lock(hard_cpu_id());
		if (rt_current->signal) {
			(*rt_current->signal)();
		}
		return;
	} else {
		sched_release_global_lock(cpuid);
	}
	return;
}


static void recover_jiffies(int irq, void *dev_id, struct pt_regs *regs)
{
	rt_global_cli();
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		rt_pend_linux_irq(TIMER_8254_IRQ);
	}
	rt_global_sti();
        BROADCAST_TO_LOCAL_TIMERS();
} 


int rt_is_hard_timer_running(void)
{
	return (rt_time_h > 0);
}


void rt_set_periodic_mode(void)
{
	stop_rt_timer();
	oneshot_timer = oneshot_running = 0;
}


void rt_set_oneshot_mode(void)
{
	stop_rt_timer();
	oneshot_timer = 1;
}


RTIME start_rt_timer(int period)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (oneshot_timer) {
		rt_request_timer(rt_timer_handler, 0, TIMER_CHIP == "APIC");
		tuned.timers_tol[0] = rt_half_tick = (tuned.latency + 1)>>1;
		oneshot_running = shot_fired = 1;
	} else {
		rt_request_timer(rt_timer_handler, period > LATCH? LATCH: period, TIMER_CHIP == "APIC");
		tuned.timers_tol[0] = rt_half_tick = (rt_times.periodic_tick + 1)>>1;
	}
	set_timer_cpu(1 << hard_cpu_id());
	rt_time_h = rt_times.tick_time + rt_half_tick;
	rt_global_restore_flags(flags);
        FREE_LOCAL_TIMERS();
	rt_request_linux_irq(TIMER_8254_IRQ, recover_jiffies, "rtai_jif_chk", recover_jiffies);
	return period;
}


#ifdef __USE_APIC__

RTIME start_rt_timer_cpuid(int period, int cpuid)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (oneshot_timer) {
		rt_request_timer_cpuid(rt_timer_handler, 0, cpuid);
		tuned.timers_tol[0] = rt_half_tick = (tuned.latency + 1)>>1;
		oneshot_running = shot_fired = 1;
	} else {
		rt_request_timer_cpuid(rt_timer_handler, period, cpuid);
		tuned.timers_tol[0] = rt_half_tick = (rt_times.periodic_tick + 1)>>1;
	}
	set_timer_cpu(1 << cpuid);
	rt_time_h = rt_times.tick_time + rt_half_tick;
        FREE_LOCAL_TIMERS();
	rt_global_restore_flags(flags);
	rt_request_linux_irq(TIMER_8254_IRQ, recover_jiffies, "rtai_jif_chk", recover_jiffies);
	return period;
}

#endif


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
	set_timer_cpu(cpu_present_map);
	oneshot_timer = oneshot_running = 0;
	rt_global_restore_flags(flags);
	rt_busy_sleep(10000000);
}


static inline RT_TASK *__whoami(void)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	ASSIGN_RT_CURRENT;
	hard_restore_flags(flags);
	return rt_current;
}


RT_TASK *rt_whoami(void)
{
	return __whoami();
}


int rt_sched_type(void)
{
	return SMP_SCHED;
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
	preempt_always = yes_no ? 1 : 0;
}


void rt_preempt_always_cpuid(int yes_no, unsigned int cpuid)
{
	rt_preempt_always(yes_no);
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


int rt_trap_handler(int vec, int signo, struct pt_regs *regs, void *dummy_data)
{
	DECLARE_RT_CURRENT;

	if (!(rt_current = __whoami())) return 0;

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

static int Preempt_Always = PREEMPT_ALWAYS;
MODULE_PARM(Preempt_Always, "i");

static int LinuxFpu = LINUX_FPU;
MODULE_PARM(LinuxFpu, "i");

static int Latency = TIMER_LATENCY;
MODULE_PARM(Latency, "i");

static int SetupTimeTIMER = TIMER_SETUP_TIME;
MODULE_PARM(SetupTimeTIMER, "i");

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
		rt_linux_task.stack = 0;
		rt_linux_task.uses_fpu = LinuxFpu ? 1 : 0;
		rt_linux_task.policy = 0;
		rt_linux_task.magic = 0;
		rt_linux_task.runnable_on_cpus = 1 << cpuid;
		rt_linux_task.state = (READY | RUNNING);
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
		rt_linux_task.prev = &rt_base_linux_task;
		rt_linux_task.resume_time = RT_TIME_END;
		rt_linux_task.tprev = rt_linux_task.tnext =
		rt_linux_task.rprev = rt_linux_task.rnext = &rt_linux_task;
		rt_linux_task.next = 0;
		rt_smp_current[cpuid] = &rt_linux_task;
	}
	set_timer_cpu(cpu_present_map);
	tuned.latency = imuldiv(Latency, tuned.cpu_freq, 1000000000);
	tuned.setup_time_TIMER_CPUNIT = imuldiv( SetupTimeTIMER, 
						 tuned.cpu_freq, 
						 1000000000);
	tuned.setup_time_TIMER_UNIT   = imuldiv( SetupTimeTIMER, 
						 TIMER_FREQ, 
						 1000000000);
	tuned.timers_tol[0] = 0;
	oneshot_timer = OneShot ? 1 : 0;
	oneshot_running = 0;
	preempt_always = Preempt_Always ? 1 : 0;
#ifdef CONFIG_PROC_FS
	rtai_proc_sched_register();
#endif
	printk("\n***** STARTING THE SMP REAL TIME %s SCHEDULER WITH %sLINUX *****", TIMER_CHIP, LinuxFpu ? "": "NO ");
	printk("\n***** FP SUPPORT AND READY FOR A %s TIMER *****", oneshot_timer ? "ONESHOT": "PERIODIC");
	printk("\n***<> LINUX TICK AT %d (HZ) <>***", HZ);
	printk("\n***<> CALIBRATED CPU FREQUENCY %lu (HZ) <>***", tuned.cpu_freq);
	printk("\n***<> CALIBRATED %s_INTERRUPT-TO-SCHEDULER LATENCY %d (ns) <>***", TIMER_CHIP, imuldiv(tuned.latency - tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
	printk("\n***<> CALIBRATED ONE SHOT %s SETUP TIME %d (ns) <>***\n\n", TIMER_CHIP, imuldiv(tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
	rt_mount_rtai();
// 0x7dd763ad == nam2num("MEMSRQ").
	if ((frstk_srq.srq = rt_request_srq(0x7dd763ad, frstk_srq_handler, 0)) < 0) {
		printk("MEM SRQ: no sysrq available.\n");
		return frstk_srq.srq;
	}
	frstk_srq.in = frstk_srq.out = 0;
	rt_request_cpu_own_irq(SCHED_IPI, rt_schedule_on_schedule_ipi);
	RT_SET_RTAI_TRAP_HANDLER(rt_trap_handler);
	return 0;
}


void cleanup_module(void)
{
	RT_SET_RTAI_TRAP_HANDLER(NULL);
	stop_rt_timer();
	while (rt_base_linux_task.next) {
		rt_task_delete(rt_base_linux_task.next);
	}
#ifdef CONFIG_PROC_FS
        rtai_proc_sched_unregister();
#endif
	while (frstk_srq.out != frstk_srq.in);
	if (rt_free_srq(frstk_srq.srq) < 0) {
		printk("MEM SRQ: frstk_srq %d illegal or already free.\n", frstk_srq.srq);
	}
	rt_free_cpu_own_irq(SCHED_IPI);
	sched_mem_end();
	rt_umount_rtai();
	printk("\n***** THE SMP REAL TIME %s BASED SCHEDULER HAS BEEN REMOVED *****\n\n", TIMER_CHIP);
}


#ifdef CONFIG_PROC_FS
/* ----------------------< proc filesystem section >----------------------*/

static int rtai_read_sched(char *page, char **start, off_t off, int count,
                           int *eof, void *data)
{
	PROC_PRINT_VARS;
	unsigned int i = 1;
	unsigned long t;
	RT_TASK *task;

	task = &rt_base_linux_task;
	PROC_PRINT("\nRTAI SMP Real Time Task Scheduler.\n\n");
	PROC_PRINT("    Calibrated CPU Frequency: %lu Hz\n", tuned.cpu_freq);
	PROC_PRINT("    Calibrated 8254 interrupt to scheduler latency: %d ns\n", imuldiv(tuned.latency - tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
	PROC_PRINT("    Calibrated one shot setup time: %d ns\n\n",
                  imuldiv(tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq));
        PROC_PRINT("Number of RT CPUs in system: %d\n\n", NR_RT_CPUS);

        PROC_PRINT("Priority  Period(ns)  FPU  Sig  State  CPUmap  Task  RT_TASK *  TIME\n" );
        PROC_PRINT("---------------------------------------------------------------------\n" );
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
			if ((!task->lnxtsk || task->is_hard) && task->exectime[1]) {
				t = 1000UL*(unsigned long)llimd(task->exectime[0], 10, tuned.cpu_freq)/(unsigned long)llimd(rdtsc() - task->exectime[1], 10, tuned.cpu_freq);
			} else {
			       t = 0;
			}
                PROC_PRINT("%-9d %-11lu %-4s %-4s 0x%-4x %-7lx %-4d  %p    %-lu\n",
                               task->priority,
                               (unsigned long)count2nano(task->period),
                               task->uses_fpu ? "Yes" : "No",
                               task->signal ? "Yes" : "No",
                               task->state,
                               task->runnable_on_cpus,
                               i, task, t);
		i++;
	}

	PROC_PRINT("TIMED\n");
	task = &rt_base_linux_task;
	while ((task = task->tnext) != &rt_base_linux_task) {
                PROC_PRINT("> %p ", task);
	}
	PROC_PRINT("\nREADY\n");
	task = &rt_base_linux_task;
	while ((task = task->rnext) != &rt_base_linux_task) {
                PROC_PRINT("> %p ", task);
	}
	PROC_PRINT("\n");

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
	counts = oneshot_timer ?
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
	ns =  oneshot_timer ?
	      llimd(ns, tuned.cpu_freq, 1000000000) :
	      llimd(ns, TIMER_FREQ, 1000000000);
	return sign ? ns : - ns;
}


RTIME count2nano_cpuid(RTIME counts, unsigned int cpuid)
{
	return count2nano(counts);
}


RTIME nano2count_cpuid(RTIME ns, unsigned int cpuid)
{
	return nano2count(ns);
}

/* +++++++++++++++++++++++++++++++ TIMINGS ++++++++++++++++++++++++++++++++++ */

RTIME rt_get_time(void)
{
	return oneshot_timer ? rdtsc(): rt_times.tick_time;
}


RTIME rt_get_time_cpuid(unsigned int cpuid)
{
	return oneshot_timer ? rdtsc(): rt_times.tick_time;
}


RTIME rt_get_time_ns(void)
{
	return oneshot_timer ?
	       llimd(rdtsc(), 1000000000, tuned.cpu_freq) :
	       llimd(rt_times.tick_time, 1000000000, TIMER_FREQ);
}


RTIME rt_get_time_ns_cpuid(unsigned int cpuid)
{
	return oneshot_timer ?
	       llimd(rdtsc(), 1000000000, tuned.cpu_freq) :
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
		task = __whoami();
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
			rt_schedule();
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_task_make_periodic_relative_ns(RT_TASK *task, RTIME start_delay, RTIME period)
{
	unsigned long flags;

	if (task->magic != RT_TASK_MAGIC) {
		return -EINVAL;
	}
	start_delay = nano2count(start_delay);
	period = nano2count(period);
	flags = rt_global_save_flags_and_cli();
	task->resume_time = rt_get_time() + start_delay;
	task->period = period;
	task->suspdepth = 0;
	if (!(task->state & DELAYED)) {
		rem_ready_task(task);
		task->state = (task->state & ~SUSPENDED) | DELAYED;
		enq_timed_task(task);
	}
	rt_schedule();
	rt_global_restore_flags(flags);
	return 0;
}


int rt_task_make_periodic(RT_TASK *task, RTIME start_time, RTIME period)
{
	unsigned long flags;

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
	rt_schedule();
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
		if ((task->tnext != &rt_base_linux_task && (new_resume_time - (task->tnext)->resume_time) > 0) || (new_resume_time - (task->tprev)->resume_time) < 0) {
			rem_timed_task(task);
			enq_timed_task(task);
			rt_schedule();
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
	flags = rt_global_save_flags_and_cli();
	task->period = new_period;
	rt_global_restore_flags(flags);
	return 0;
}


void rt_task_drift_wait_period(void)
{
	DECLARE_RT_CURRENT;
	RTIME now;
	long flags;

	flags = rt_global_save_flags_and_cli();
	now = rt_get_time();
	ASSIGN_RT_CURRENT;
	rt_current->resume_time += rt_current->period;
	if ((rt_current->resume_time + rt_half_tick) > now) {
		rt_current->state |= DELAYED;
		rt_schedule();
	} else {
		rt_current->resume_time = now;
	}
	rt_global_restore_flags(flags);
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
	if ((task->state & ~(READY | RUNNING)) &&  (task->state &= ~DELAYED) == READY) {
		enq_ready_task(task);
		rt_schedule();
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
	task->blocked_on         = NOTHING;
}


static __volatile__ inline void pass_prio(RT_TASK *to, RT_TASK *from)
{
	QUEUE *q;
	from->prio_passed_to = to;
	while (to && to->priority > from->priority) {
		to->priority = from->priority;
		if (to->state && !(to->state & ~(READY | RUNNING))) {
			(to->rprev)->rnext = to->rnext;
			(to->rnext)->rprev = to->rprev;
			enq_ready_task(to);
		} else if ((q = to->blocked_on) && !((to->state & SEMAPHORE) && ((SEM *)q)->qtype)) {
			(to->queue.prev)->next = to->queue.next;
			(to->queue.next)->prev = to->queue.prev;
			while ((q = q->next) != to->blocked_on && (q->task)->priority <= to->priority);
			q->prev = (to->queue.prev = q->prev)->next  = &(to->queue);
			to->queue.next = q;
		}
		to = to->prio_passed_to;
	}
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
	int sched;
	QUEUE *q;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	sched = 0;
	q = &(sem->queue);
	flags = rt_global_save_flags_and_cli();
	sem->magic = 0;
	while ((q = q->next) != &(sem->queue) && (task = q->task)) {
		rem_timed_task(task);
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			enq_ready_task(task);	
			sched = 1;
		}
	}
	if ((task = sem->owndby) && sem->type > 0) {
		if (task->owndres & SEMHLF) {
			--task->owndres;
		}
		if (!task->owndres) {
			sched = renq_ready_task(task, task->base_priority);
		} else if (!(task->owndres & SEMHLF)) {
			int priority;
			sched = renq_ready_task(task, task->base_priority > (priority = ((task->msg_queue.next)->task)->priority) ? priority : task->base_priority);
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
	}
	if (sched) {
		rt_schedule();
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_signal(SEM *sem)
{
	unsigned long flags;
	RT_TASK *task;

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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			enq_ready_task(task);	
			if (sem->type <= 0) {
				rt_schedule();
				rt_global_restore_flags(flags);
				return 0;
			}
		}
	}
	if (sem->type > 0) {
		DECLARE_RT_CURRENT;
		int sched;
		ASSIGN_RT_CURRENT;
		sem->owndby = 0;
		if (rt_current->owndres & SEMHLF) {
			--rt_current->owndres;
		}
		if (!rt_current->owndres) {
			sched = renq_ready_task(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_ready_task(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
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
			rt_schedule();
		}	
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_broadcast(SEM *sem)
{
	unsigned long flags;
	RT_TASK *task;
	int sched;
	QUEUE *q;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	sched = 0;
	q = &(sem->queue);
	flags = rt_global_save_flags_and_cli();
	while ((q = q->next) != &(sem->queue)) {
		dequeue_blocked(task = q->task);
		rem_timed_task(task);
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			enq_ready_task(task);
			sched = 1;
		}
	}
	sem->count = 0;
	sem->queue.prev = sem->queue.next = &(sem->queue);
	if (sched) {
		rt_schedule();
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
		if (sem->type > 0) {
			if (sem->owndby == rt_current) {
				sem->type++;
				rt_global_restore_flags(flags);
				return count;
			}
			pass_prio(sem->owndby, rt_current);
		}
		sem->count--;
		rt_current->state |= SEMAPHORE;
		rem_ready_current(rt_current);
		enqueue_blocked(rt_current, &sem->queue, sem->qtype);
		rt_schedule();
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
			if (sem->type > 0) {
				if (sem->owndby == rt_current) {
					sem->type++;
					rt_global_restore_flags(flags);
					return 0;
				}
				pass_prio(sem->owndby, rt_current);
			}
			sem->count--;
			rt_current->state |= (SEMAPHORE | DELAYED);
			rem_ready_current(rt_current);
			enqueue_blocked(rt_current, &sem->queue, sem->qtype);
			enq_timed_task(rt_current);
			rt_schedule();
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
	return rt_sem_wait_until(sem, (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay);
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
	if ((1 - sem->count) > (int)sem->owndby) {
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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);	
			rt_schedule();
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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);	
			rt_schedule();
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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);	
			rt_schedule();
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
	return rt_send_until(task, msg, (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay);
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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
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
	rt_schedule();
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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
			enq_ready_task(task);
		}
		rt_current->state |= RETURN;
		rem_ready_current(rt_current);
		rt_current->msg_queue.task = task;
		rt_schedule();
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
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RECEIVE | DELAYED)) == READY) {
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
	rt_schedule();
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
	return rt_rpc_until(task, to_do, result, (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay);
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
			sched = renq_ready_task(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_ready_task(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
		} else {
			sched = 0;
		}
		task->msg = result;
		task->msg_queue.task = task;
		rem_timed_task(task);
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(RETURN | DELAYED)) == READY) {
			enq_ready_task(task);	
			rt_schedule();
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

	rt_current = __whoami();
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
			if ((task->state & ~(READY | RUNNING)) && (task->state  &=  ~(SEND | DELAYED)) == READY) {
				enq_ready_task(task);	
				rt_schedule();
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
			if ((task->state & ~(READY | RUNNING)) && (task->state  &= ~(SEND | DELAYED)) == READY) {
				enq_ready_task(task);	
				rt_schedule();
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
			if ((task->state & ~(READY | RUNNING)) && (task->state  &= ~(SEND | DELAYED)) == READY) {
				enq_ready_task(task);	
				rt_schedule();
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
	return rt_receive_until(task, msg, (oneshot_timer ? rdtsc(): rt_times.tick_time) + delay);
}

/* ++++++++++++++++++++++++++ EXTENDED MESSAGES +++++++++++++++++++++++++++++++
COPYRIGHT (C) 2002  Pierre Cloutier  (pcloutier@poseidoncontrols.com)
                    Paolo Mantegazza (mantegazza@aero.polimi.it)
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#define SET_MCB \
	do { \
		mcb.sbuf = smsg; \
		mcb.sbytes = ssize; \
		mcb.rbuf = rmsg; \
		mcb.rbytes = rsize; \
	} while (0)

RT_TASK *rt_rpcx(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (task) {
		struct mcb_t mcb;
		SET_MCB;
		return rt_rpc(task, (unsigned int)&mcb, &mcb.rbytes);
	}
	return 0;
}


RT_TASK *rt_rpcx_if(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (task) {
		struct mcb_t mcb;
		SET_MCB;
		return rt_rpc_if(task, (unsigned int)&mcb, &mcb.rbytes);
	}
	return 0;
}


RT_TASK *rt_rpcx_until(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time)
{
	if (task) {
		struct mcb_t mcb;
		SET_MCB;
		return rt_rpc_until(task, (unsigned int)&mcb, &mcb.rbytes, time);
	}
	return 0;
}


RT_TASK *rt_rpcx_timed(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay)
{
	if (task) {
		struct mcb_t mcb;
		SET_MCB;
		return rt_rpc_timed(task, (unsigned int)&mcb, &mcb.rbytes, delay);
	}
	return 0;
}


RT_TASK *rt_sendx(RT_TASK *task, void *msg, int size) 
{
	return rt_rpcx(task, msg, 0, size, -1);
}


RT_TASK *rt_sendx_if(RT_TASK *task, void *msg, int size)
{
	return rt_rpcx_if(task, msg, 0, size, -1);
}


RT_TASK *rt_sendx_until(RT_TASK *task, void *msg, int size, RTIME time)
{
	return rt_rpcx_until(task, msg, 0, size, -1, time);
}


RT_TASK *rt_sendx_timed(RT_TASK *task, void *msg, int size, RTIME delay)
{
	return rt_rpcx_timed(task, msg, 0, size, -1, delay);
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

#define DO_RCV_MSG \
	do { \
		if ((*len = size <= mcb->sbytes ? size : mcb->sbytes)) { \
			memcpy(msg, mcb->sbuf, *len); \
		} \
		if (mcb->rbytes < 0) { \
			rt_return(task, 0); \
		} \
	} while (0)


RT_TASK *rt_evdrpx(RT_TASK *task, void *msg, int size, int *len)
{
	struct mcb_t *mcb;
	if ((task = rt_evdrp(task, (unsigned int *)&mcb))) {
		if ((*len = size <= mcb->sbytes ? size : mcb->sbytes)) {
			memcpy(msg, mcb->sbuf, *len);
		}
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex(RT_TASK *task, void *msg, int size, int *len)
{
	struct mcb_t *mcb;
	if ((task = rt_receive(task, (unsigned int *)&mcb))) {
		DO_RCV_MSG;
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex_if(RT_TASK *task, void *msg, int size, int *len)
{
	struct mcb_t *mcb;
	if ((task = rt_receive_if(task, (unsigned int *)&mcb))) {
		DO_RCV_MSG;
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex_until(RT_TASK *task, void *msg, int size, int *len, RTIME time)
{
	struct mcb_t *mcb;
	if ((task = rt_receive_until(task, (unsigned int *)&mcb, time))) {
		DO_RCV_MSG;
		return task;
	}
	return 0;
}


RT_TASK *rt_receivex_timed(RT_TASK *task, void *msg, int size, int *len, RTIME delay)
{
	struct mcb_t *mcb;
	if ((task = rt_receive_timed(task, (unsigned int *)&mcb, delay))) {
		DO_RCV_MSG;
		return task;
	}
	return 0;
}

/* +++++++++++++++++++++++++++++ MAIL BOXES ++++++++++++++++++++++++++++++++ */

static inline void mbx_signal(MBX *mbx)
{
	unsigned long flags;
	RT_TASK *task;

	flags = rt_global_save_flags_and_cli();
	if ((task = mbx->waiting_task)) {
		rem_timed_task(task);
		mbx->waiting_task = NOTHING;
		task->prio_passed_to = NOTHING;
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(MBXSUSP | DELAYED)) == READY) {
			enq_ready_task(task);	
			if (mbx->sndsem.type <= 0) {
				rt_schedule();
				rt_global_restore_flags(flags);
				return;
			}
		}
	}
	if (mbx->sndsem.type > 0) {
		DECLARE_RT_CURRENT;
		int sched;
		ASSIGN_RT_CURRENT;
		mbx->owndby = 0;
		if (rt_current->owndres & SEMHLF) {
			--rt_current->owndres;
		}
		if (!rt_current->owndres) {
			sched = renq_ready_task(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_ready_task(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
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
                        rt_schedule();
                }
	}
	rt_global_restore_flags(flags);
}

#define RT_MBX_MAGIC 0x3ad46e9b

static inline int mbx_wait(MBX *mbx, int *fravbs, RT_TASK *rt_current)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (!(*fravbs) && !mbx->waiting_task) {
		if (mbx->sndsem.type > 0) {
			pass_prio(mbx->owndby, rt_current);
		}
		rt_current->state |= MBXSUSP;
		rem_ready_current(rt_current);
		mbx->waiting_task = rt_current;
		rt_schedule();
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
	if (!(*fravbs) && !mbx->waiting_task) {
		mbx->waiting_task = rt_current;
		if ((rt_current->resume_time = time) > rt_time_h) {
			if (mbx->sndsem.type > 0) {
				pass_prio(mbx->owndby, rt_current);
			}
			rt_current->state |= (MBXSUSP | DELAYED);
			rem_ready_current(rt_current);
			enq_timed_task(rt_current);
			rt_schedule();
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
	rt_current = __whoami();
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
	rt_current = __whoami();
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
	return rt_mbx_send_until(mbx, msg, msg_size, (oneshot_timer ? rdtsc() : rt_times.tick_time) + delay);
}


int rt_mbx_receive(MBX *mbx, void *msg, int msg_size)
{
	DECLARE_RT_CURRENT;

	CHK_MBX_MAGIC;
	if (rt_sem_wait(&mbx->rcvsem) > 1) {
		return msg_size;
	}
	rt_current = __whoami();
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
	rt_current = __whoami();
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

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* +++++++++++++++++++++++++++ SECRET BACK DOORS ++++++++++++++++++++++++++++ */

void (*dnepsus_trxl)(void) = rt_schedule;

void emuser_trxl(RT_TASK *new_task)
{
	DECLARE_RT_CURRENT;
	int cpuid;

	if ((new_task->state |= READY) == READY) {
		(rt_current = rt_smp_current[cpuid = hard_cpu_id()])->state = READY;
		new_task->state = (READY | RUNNING);
		enq_ready_task(new_task);	
		rt_switch_to_real_time(cpuid);
		save_cr0_and_clts(linux_cr0);
		if (rt_current->uses_fpu) {
			save_fpenv(rt_current->fpu_reg);
			if (new_task->uses_fpu) {
				restore_fpenv(new_task->fpu_reg);
			}
		} else if (new_task->uses_fpu) {
			restore_fpenv(new_task->fpu_reg);
		}
		KEXECTIME();
		rt_exchange_tasks(rt_smp_current[cpuid], new_task);
		if (rt_current->signal) {
			sched_release_global_lock(hard_cpu_id());
			(*rt_current->signal)();
			sched_get_global_lock(cpuid);
		}
	}
}

void tratser_trxl(RT_TASK *task, void (*rt_thread)(int))
{
#define data ((int)task)
	task->stack = task->bstack;
	init_arch_stack();
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

RT_TASK *rt_get_base_linux_task(RT_TASK **base_linux_tasks)
{
        base_linux_tasks[0] = &rt_base_linux_task;
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

RT_TASK **rt_register_watchdog(RT_TASK *wd, int cpuid)
{
    	RT_TASK *task;

	if (wdog_task[cpuid]) return (RT_TASK**) -EBUSY;
	task = &rt_base_linux_task;
	while ((task = task->next)) {
	    	if (task != wd && task->priority == RT_HIGHEST_PRIORITY) {
		    	return (RT_TASK**) -EBUSY;
		}
	}
	wdog_task[cpuid] = wd;
	return rt_smp_current;
}

void rt_deregister_watchdog(RT_TASK *wd, int cpuid)
{
    	if (wdog_task[cpuid] != wd) return;
	wdog_task[cpuid] = NULL;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

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

void rt_wake_up_timed_tasks(void)
{
	wake_up_timed_tasks();
}

void rt_rem_timed_task(RT_TASK *task)
{
	rem_timed_task(task);
}

void rt_dequeue_blocked(RT_TASK *task)
{
	dequeue_blocked(task);
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

int rt_nanosleep(struct timespec *rqtp, struct timespec *rmtp)
{
	RTIME expire;

	if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec < 0) {
		return -EINVAL;
	}
	rt_sleep_until(expire = rt_get_time() + timespec2count(rqtp));
	if ((expire -= rt_get_time()) > 0) {
		if (rmtp) {
			count2timespec(expire, rmtp);
		}
		return -EINTR;
	}
	return 0;
}
