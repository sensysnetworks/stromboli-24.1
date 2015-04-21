/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#ifndef _RTAI_SCHED_H_
#define _RTAI_SCHED_H_

#ifdef __KERNEL__
#include <linux/wait.h>
#include <rtai.h>
#include <rtai_types.h>
#include <rt_mem_mgr.h>
#include <linux/time.h>
#include <linux/errno.h>
#else
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#endif

#ifdef INTERFACE_TO_LINUX
#define RT_LINUX_PRIORITY 0x7fffFfff
#endif

#define UP_SCHED   1
#define SMP_SCHED  2
#define MUP_SCHED  3

#define RT_HIGHEST_PRIORITY         0
#define RT_LOWEST_PRIORITY 0x3fffFfff

#define READY        1
#define SUSPENDED    2
#define DELAYED      4
#define SEMAPHORE    8
#define SEND        16
#define RECEIVE     32
#define RPC         64
#define RETURN     128
#define RUNNING    256
#define MBXSUSP    512

#define PRIO_Q    0 
#define FIFO_Q    4 
#define RES_Q     3

#define BIN_SEM   1 
#define CNT_SEM   2 
#define RES_SEM   3

#define RT_SCHED_FIFO  0 
#define RT_SCHED_RR    1 

struct rt_queue {
	struct rt_queue *prev;
	struct rt_queue *next;
	struct rt_task_struct *task;
};

typedef struct rt_queue QUEUE;

struct rt_semaphore {
	struct rt_queue queue; //must be first in struct
	int magic;
	int type;
	int count;
	struct rt_task_struct *owndby;
	int qtype;
};

typedef struct rt_semaphore SEM;

typedef SEM CND;

struct rdwr_lock { SEM wrmtx, wrsem, rdsem; };
typedef struct rdwr_lock RWL;

struct rt_spl_t { void *owndby; int count; unsigned long flags; };
typedef struct rt_spl_t SPL;

struct rt_ExitHandler { // Exit handler functions are called like C++ destructors in rt_task_delete()
	struct rt_ExitHandler *nxt;
	void (*fun) (void *arg1, int arg2);
	void *arg1;
	int   arg2;
};

typedef struct rt_ExitHandler XHDL;

struct mcb_t { void *sbuf; int sbytes; void *rbuf; int rbytes; };

struct rt_task_struct {
	int *stack;
	int uses_fpu;
	int magic;
	volatile int state;
	unsigned long runnable_on_cpus;
	int *stack_bottom;
	volatile int priority;
	int base_priority;
	int policy;
	int sched_lock_priority;
	struct rt_task_struct *prio_passed_to;
	RTIME period;
	RTIME resume_time;
	RTIME yield_time;
	int rr_quantum;
	int rr_remaining;
	int suspdepth;
	struct rt_queue queue;
	int owndres;
	struct rt_queue *blocked_on;
	struct rt_queue msg_queue;
        int tid;			/* trace ID */
	unsigned int msg;
	struct rt_queue ret_queue;
	void (*signal)(void);
	FPU_ENV fpu_reg;
	struct rt_task_struct *prev;
	struct rt_task_struct *next;
	struct rt_task_struct *tprev;
	struct rt_task_struct *tnext;
	struct rt_task_struct *rprev;
	struct rt_task_struct *rnext;
// appended for calls from LINUX 
	int *fun_args, *bstack;
	struct task_struct *lnxtsk;
	long long retval;
	char *msg_buf[2];
	int max_msg_size[2];
	char task_name[16];
	void *system_data_ptr;
	struct rt_task_struct *nextp;
	struct rt_task_struct *prevp;

// Added to support user specific trap handlers.
        RT_TRAP_HANDLER task_trap_handler[RTAI_NR_TRAPS];

// Added from rtai-22
	void (*usp_signal)(void);
	volatile unsigned long pstate;
	unsigned long usp_flags;
	unsigned long usp_flags_mask;
	unsigned long force_soft;
	volatile int is_hard;

// Added to terminate qBlks
	void *tick_queue;
// Added to terminate re-entry of user space functions
	void *trap_handler_data; 
	int trap_signo;
// For use by watchdog
	int resync_frame;
// For use by exit handler functions
	XHDL *ExitHook;
	int linux_signal;
	int errno;
	void (*linux_signal_handler)(int sig);
	RTIME exectime[2];
	struct mcb_t mcb;
} __attribute__ ((__aligned__ (16)));

struct proxy_t { RT_TASK *receiver; int nmsgs, nbytes; char *msg; };

struct rt_mailbox {
	int magic;
	SEM sndsem, rcvsem;
	RT_TASK *waiting_task, *owndby;
	char *bufadr;
	int size, fbyte, lbyte, avbs, frbs;
	spinlock_t lock;
};

typedef struct rt_mailbox MBX;


#ifdef __KERNEL__

#define RT_TASK_MAGIC 0x754d2774

#define SEM_TIMOUT (0xFffe)

#define MSG_ERR ((RT_TASK *)0xFfff)

extern int rt_task_init(RT_TASK *task, void (*rt_thread)(int), int data,
				int stack_size, int priority, int uses_fpu,
				void(*signal)(void));

extern int rt_task_init_cpuid(RT_TASK *task, void (*rt_thread)(int), int data,
				int stack_size, int priority, int uses_fpu,
				void(*signal)(void), unsigned int run_on_cpu);

extern void rt_set_runnable_on_cpus(RT_TASK *task, unsigned int cpu_mask);

extern void rt_set_runnable_on_cpuid(RT_TASK *task, unsigned int cpuid);

extern void rt_set_sched_policy(RT_TASK *task, int policy, int rr_quantum_ns);

extern int rt_task_delete(RT_TASK *task);

extern int rt_get_task_state(RT_TASK *task);

extern void rt_gettimeorig(RTIME time_orig[]);

extern int rt_get_timer_cpu(void);

extern int rt_is_hard_timer_running(void);

extern void rt_set_periodic_mode(void);

extern void rt_set_oneshot_mode(void);

extern RTIME start_rt_timer(int period);

extern RTIME start_rt_timer_cpuid(int period, int cpuid);

#define start_rt_timer_ns(period) start_rt_timer(nano2count((period)))

extern void start_rt_apic_timers(struct apic_timer_setup_data *setup_mode, unsigned int rcvr_jiffies_cpuid);

extern void stop_rt_timer(void);

extern RT_TASK *rt_whoami(void);

extern int rt_sched_type(void);

extern int rt_task_signal_handler(RT_TASK *task, void (*handler)(void));

extern int rt_task_use_fpu(RT_TASK *task, int use_fpu_flag);
  
extern void rt_linux_use_fpu(int use_fpu_flag);

extern void rt_preempt_always(int yes_no);

extern void rt_preempt_always_cpuid(int yes_no, unsigned int cpuid);

extern RTIME count2nano(RTIME timercounts);

extern RTIME nano2count(RTIME nanosecs);
  
extern RTIME count2nano_cpuid(RTIME timercounts, unsigned int cpuid);

extern RTIME nano2count_cpuid(RTIME nanosecs, unsigned int cpuid);
  
extern RTIME rt_get_time(void);

extern RTIME rt_get_time_cpuid(unsigned int cpuid);

extern RTIME rt_get_time_ns(void);

extern RTIME rt_get_time_ns_cpuid(unsigned int cpuid);

extern RTIME rt_get_cpu_time_ns(void);

extern int rt_get_prio(RT_TASK *task);

extern int rt_get_inher_prio(RT_TASK *task);

extern void rt_spv_RMS(int cpuid);

extern int rt_change_prio(RT_TASK *task, int priority);

extern void rt_sched_lock(void);

extern void rt_sched_unlock(void);

extern void rt_task_yield(void);

extern int rt_task_suspend(RT_TASK *task);

extern int rt_task_resume(RT_TASK *task);

extern int rt_task_make_periodic_relative_ns(RT_TASK *task, RTIME start_delay, RTIME period);

extern int rt_task_make_periodic(RT_TASK *task, RTIME start_time, RTIME period);

void rt_task_set_resume_end_times(RTIME resume, RTIME end);

int rt_set_resume_time(RT_TASK *task, RTIME new_resume_time);

int rt_set_period(RT_TASK *task, RTIME new_period);

extern void rt_task_wait_period(void);

extern RTIME next_period(void);

extern void rt_busy_sleep(int nanosecs);

extern void rt_sleep(RTIME delay);

extern void rt_sleep_until(RTIME time);

extern int rt_task_wakeup_sleeping(RT_TASK *task);

extern void rt_typed_sem_init(SEM *sem, int value, int type);

extern void rt_sem_init(SEM *sem, int value);

extern int rt_sem_delete(SEM *sem);

extern int rt_sem_signal(SEM *sem);

extern int rt_sem_broadcast(SEM *sem);

extern int rt_sem_wait(SEM *sem);

extern int rt_sem_wait_if(SEM *sem);

extern int rt_cntsem_wait_if_and_lock(SEM *sem);

extern int rt_sem_wait_until(SEM *sem, RTIME time);

extern int rt_sem_wait_timed(SEM *sem, RTIME delay);

extern int rt_sem_wait_barrier(SEM *sem);

extern int rt_sem_count(SEM *sem);

extern int rt_cond_wait(CND *cnd, SEM *mtx);

extern int rt_cond_wait_until(CND *cnd, SEM *mtx, RTIME time);

extern int rt_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay);

extern int rt_rwl_init(RWL *rwl);

extern int rt_rwl_delete(RWL *rwl);

extern int rt_rwl_rdlock(RWL *rwl);

extern int rt_rwl_rdlock_if(RWL *rwl);

extern int rt_rwl_rdlock_until(RWL *rwl, RTIME time);

extern int rt_rwl_rdlock_timed(RWL *rwl, RTIME delay);

extern int rt_rwl_wrlock(RWL *rwl);

extern int rt_rwl_wrlock_if(RWL *rwl);

extern int rt_rwl_wrlock_until(RWL *rwl, RTIME time);

extern int rt_rwl_wrlock_timed(RWL *rwl, RTIME delay);

extern int rt_rwl_unlock(RWL *rwl);

extern int rt_spl_init(SPL *spl);

extern int rt_spl_delete(SPL *spl);

extern int rt_spl_lock(SPL *spl);

extern int rt_spl_lock_if(SPL *spl);

extern int rt_spl_lock_timed(SPL *spl, unsigned long ns);

extern int rt_spl_unlock(SPL *spl);

extern RT_TASK *rt_send(RT_TASK *task, unsigned int msg);

extern RT_TASK *rt_send_if(RT_TASK *task, unsigned int msg);

extern RT_TASK *rt_send_until(RT_TASK *task, unsigned int msg, RTIME time);

extern RT_TASK *rt_send_timed(RT_TASK *task, unsigned int msg, RTIME delay);

extern RT_TASK *rt_evdrp(RT_TASK *task, unsigned int *msg);

extern RT_TASK *rt_receive(RT_TASK *task, unsigned int *msg);

extern RT_TASK *rt_receive_if(RT_TASK *task, unsigned int *msg);

extern RT_TASK *rt_receive_until(RT_TASK *task, unsigned int *msg, RTIME time);

extern RT_TASK *rt_receive_timed(RT_TASK *task, unsigned int *msg, RTIME delay);

extern RT_TASK *rt_rpc(RT_TASK *task, unsigned int to_do, unsigned int *result);

extern RT_TASK *rt_rpc_if(RT_TASK *task, unsigned int to_do, unsigned int *result);

extern RT_TASK *rt_rpc_until(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME time);

extern RT_TASK *rt_rpc_timed(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME delay);

extern int rt_isrpc(RT_TASK *task);

extern RT_TASK *rt_return(RT_TASK *task, unsigned int result);

extern RT_TASK *rt_rpcx(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize);

extern RT_TASK *rt_rpcx_if(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize);

extern RT_TASK *rt_rpcx_until(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time);

extern RT_TASK *rt_rpcx_timed(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay);

extern RT_TASK *rt_sendx(RT_TASK *task, void *msg, int size);

extern RT_TASK *rt_sendx_if(RT_TASK *task, void *msg, int size);

extern RT_TASK *rt_sendx_until(RT_TASK *task, void *msg, int size, RTIME time);

extern RT_TASK *rt_sendx_timed(RT_TASK *task, void *msg, int size, RTIME delay);

extern RT_TASK *rt_returnx(RT_TASK *task, void *msg, int size);

#define rt_isrpcx(task) rt_isrpc(task)

extern RT_TASK *rt_evdrpx(RT_TASK *task, void *msg, int size, int *len);

extern RT_TASK *rt_receivex(RT_TASK *task, void *msg, int size, int *len);

extern RT_TASK *rt_receivex_if(RT_TASK *task, void *msg, int size, int *len);

extern RT_TASK *rt_receivex_until(RT_TASK *task, void *msg, int size, int *len, RTIME time);

extern RT_TASK *rt_receivex_timed(RT_TASK *task, void *msg, int size, int *len, RTIME delay);

extern int rt_typed_mbx_init(MBX *mbx, int size, int qtype);

extern int rt_mbx_init(MBX *mbx, int size);

extern int rt_mbx_delete(MBX *mbx);

extern int rt_mbx_send(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_send_wp(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_send_if(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_send_until(MBX *mbx, void *msg, int msg_size, RTIME time);

extern int rt_mbx_send_timed(MBX *mbx, void *msg, int msg_size, RTIME delay);

extern int rt_mbx_ovrwr_send(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_evdrp(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_receive(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_receive_wp(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_receive_if(MBX *mbx, void *msg, int msg_size);

extern int rt_mbx_receive_until(MBX *mbx, void *msg, int msg_size, RTIME time);

extern int rt_mbx_receive_timed(MBX *mbx, void *msg, int msg_size, RTIME delay);

#define exist(name)  rt_get_adr(nam2num(name))

extern RT_TASK *rt_named_task_init(const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void));

extern RT_TASK *rt_named_task_init_cpuid(const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void), unsigned int run_on_cpu);

extern SEM *rt_typed_named_sem_init(const char *sem_name, int value, int type);

extern MBX *rt_typed_named_mbx_init(const char *mbx_name, int size, int qtype);

#define rt_named_sem_init(sem_name, value)  rt_typed_named_sem_init(sem_name, value, CNT_SEM)

#define rt_named_mbx_init(mbx_name, size)  rt_typed_named_mbx_init(mbx_name, size, FIFO_Q)

extern int rt_named_task_delete(RT_TASK *task);

extern int rt_named_sem_delete(SEM *sem);

extern int rt_named_mbx_delete(MBX *mbx);

// a few defines and inlines to make RTAI resemble POSIX more closely

static inline RTIME timeval2count(struct timeval *t)
{
        return nano2count(t->tv_sec*1000000000LL + t->tv_usec*1000);
}

static inline void count2timeval(RTIME rt, struct timeval *t)
{
        t->tv_sec = ulldiv(count2nano(rt), 1000000000, (unsigned long *)&t->tv_usec);
        t->tv_usec /= 1000;
}

static inline RTIME timespec2count(const struct timespec *t)
{
        return nano2count(t->tv_sec*1000000000LL + t->tv_nsec);
}

static inline void count2timespec(RTIME rt, struct timespec *t)
{
        t->tv_sec = ulldiv(count2nano(rt), 1000000000, (unsigned long *)&t->tv_nsec);
}

typedef struct rt_semaphore psem_t;

static inline int rt_psem_init(psem_t *sem, int pshared, unsigned int value)
{
	if (value < SEM_TIMOUT) {
		rt_typed_sem_init(sem, value, pshared | PRIO_Q);
		return 0;
	}
	return -EINVAL;
}

static inline int rt_psem_destroy(psem_t *sem)
{
	if (rt_sem_wait_if(sem) >= 0) {
		rt_sem_signal(sem);
		return rt_sem_delete(sem);
	}
	return -EBUSY;
}

static inline int rt_psem_wait(psem_t *sem)
{
	return rt_sem_wait(sem) < SEM_TIMOUT ? 0 : -1;
}

static inline int rt_psem_timedwait(psem_t *sem, struct timespec *abstime)
{
	return rt_sem_wait_until(sem, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -1;
}

static inline int rt_psem_trywait(psem_t *sem)
{
	return rt_sem_wait_if(sem) > 0 ? 0 : -EAGAIN;
}

static inline int rt_psem_post(psem_t *sem)
{
	return rt_sem_wait(sem) < SEM_TIMOUT ? 0 : -ERANGE;
}

static inline int rt_psem_getvalue(psem_t *sem, int *sval)
{
	if ((*sval = rt_sem_wait_if(sem)) > 0) {
		rt_sem_signal(sem);
	}
	return 0;
}

typedef struct rt_semaphore pmutex_t;

static inline int rt_pmutex_init(pmutex_t *mutex, void *mutexattr)
{
	rt_typed_sem_init(mutex, 1, RES_SEM);
	return 0;
}

static inline int rt_pmutex_destroy(pmutex_t *mutex)
{
	if (rt_sem_wait_if(mutex) > 0) {
		rt_sem_signal(mutex);
		return rt_sem_delete(mutex);
	}
	return -EBUSY;
}

static inline int rt_pmutex_lock(pmutex_t *mutex)
{
	return rt_sem_wait(mutex) < SEM_TIMOUT ? 0 : -EINVAL;
}

static inline int rt_pmutex_trylock(pmutex_t *mutex)
{
	return rt_sem_wait_if(mutex) > 0 ? 0 : -EBUSY;
}

static inline int rt_pmutex_timedlock(pmutex_t *sem, struct timespec *abstime)
{
	return rt_sem_wait_until(sem, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -1;
}

static inline int rt_pmutex_unlock(pmutex_t *mutex)
{
	return rt_sem_signal(mutex) < SEM_TIMOUT ? 0 : -EINVAL;
}

#define rt_mutex_init(mtx)             rt_typed_sem_init(mtx, 1, RES_SEM)
#define rt_mutex_delete(mtx)           rt_sem_delete(mtx)
#define rt_mutex_destroy(mtx)          rt_sem_delete(mtx)
#define rt_mutex_trylock(mtx)          rt_sem_wait_if(mtx)
#define rt_mutex_lock(mtx)             rt_sem_wait(mtx)
#define rt_mutex_timedlock(mtx, time)  rt_sem_wait_until(mtx, time)
#define rt_mutex_unlock(mtx)           rt_sem_signal(mtx)

#define rt_cond_init(cnd)                  rt_typed_sem_init(cnd, 0, BIN_SEM)
#define rt_cond_delete(cnd)                rt_sem_delete(cnd)
#define rt_cond_destroy(cnd)               rt_sem_delete(cnd)
#define rt_cond_broadcast(cnd)             rt_sem_broadcast(cnd)
#define rt_cond_signal(cnd)                rt_sem_signal(cnd)
#define rt_cond_timedwait(cnd, mtx, time)  rt_cond_wait_until(cnd, mtx, time)

#if 0
static inline int rt_cond_wait(CND *cnd, SEM *mtx)
{
	rt_sem_signal(mtx);
	rt_sem_wait(cnd);
	rt_sem_wait(mtx);
	return 0;
}

static inline int rt_cond_wait_until(CND *cnd, SEM *mtx, RTIME time)
{
	int semret;
	semret = 0;
	rt_sem_signal(mtx);
	semret = rt_sem_wait_until(cnd, time);
	rt_sem_wait(mtx);
	return semret >= SEM_TIMOUT ? -1 : 0;
}

static inline int rt_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay)
{
	int semret;
	semret = 0;
	rt_sem_signal(mtx);
	semret = rt_sem_wait_timed(cnd, delay);
	rt_sem_wait(mtx);
	return semret >= SEM_TIMOUT ? -1 : 0;
}
#endif

static inline void __call_exit_handlers(RT_TASK *task)
{
	XHDL *pt, *tmp;

	pt = task->ExitHook; // Initialise ExitHook in rt_task_init()
	while ( pt ) {
		(*pt->fun) (pt->arg1, pt->arg2);
		tmp = pt;
		pt  = pt->nxt;
		rt_free(tmp);
	}
	task->ExitHook = 0;
}

static inline XHDL *__set_exit_handler(RT_TASK *task, void (*fun) (void *, int), void *arg1, int arg2)
{
	XHDL *p;

	// exit handler functions are automatically executed at terminattion time by rt_task_delete()
	// in the reverse order they were created (like C++ destructors behave).
	if (task->magic != RT_TASK_MAGIC) return 0;
	if (!(p = (XHDL *) rt_malloc (sizeof(XHDL)))) return 0;
	p->fun  = fun;
	p->arg1 = arg1;
	p->arg2 = arg2;
	p->nxt  = task->ExitHook;
	return (task->ExitHook = p);
}

#ifndef CONFIG_PPC
extern RT_TRAP_HANDLER rt_set_task_trap_handler( RT_TASK *task, unsigned int vec, RT_TRAP_HANDLER handler);
#endif

// Undocumented calls, use them only if you know what they are for.

extern RT_TASK *rt_get_base_linux_task(RT_TASK **base_linux_task);

#define RT_SCHED() \
	do { extern void (*dnepsus_trxl)(void); (*dnepsus_trxl)(); } while (0)

extern RT_TASK *rt_alloc_dynamic_task(void);

extern void rt_enq_ready_edf_task(RT_TASK *ready_task);

extern void rt_enq_ready_task(RT_TASK *ready_task);

extern int rt_renq_ready_task(RT_TASK *ready_task, int priority);

extern void rt_rem_ready_task(RT_TASK *task);

extern void rt_rem_ready_current(RT_TASK *rt_current);

extern void rt_enq_timed_task(RT_TASK *timed_task);

extern void rt_rem_timed_task(RT_TASK *task);

extern void rt_dequeue_blocked(RT_TASK *task);

extern RT_TASK **rt_register_watchdog(RT_TASK *wdog, int cpuid);

extern void rt_deregister_watchdog(RT_TASK *wdog, int cpuid);

// End of undocumented services.

#endif

#endif
