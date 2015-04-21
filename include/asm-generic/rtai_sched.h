
#ifndef _RTAI_ASM_GENERIC_RTAI_SCHED_H_
#define _RTAI_ASM_GENERIC_RTAI_SCHED_H_

/* useful in stringizing macro expansions */
#define __STR(x) #x
#define STR(x) __STR(x)

/* add some assembly code here */
#define rt_switch_to(tsk)
#define rt_exchange_tasks(oldtask, newtask)
#define init_arch_stack() do{}while(0)
#define init_fp_env() do{}while(0)

#define DEFINE_LINUX_CR0 static unsigned long linux_cr0;

#define DEFINE_LINUX_SMP_CR0 static unsigned long linux_smp_cr0[NR_RT_CPUS];

static inline void *get_stack_pointer(void)
{
	return NULL;
}

#define RT_SET_RTAI_TRAP_HANDLER(x) rt_set_rtai_trap_handler(x)

#endif

