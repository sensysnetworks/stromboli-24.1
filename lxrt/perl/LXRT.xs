
#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <version.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#ifdef __cplusplus
}
#endif


MODULE = LXRT            PACKAGE = LXRT

#long long
#rtai_lxrt(srq, arg)
#	int    srq
#	void * arg
#    CODE:
##if RTAI_VERSION_CODE && RTAI_VERSION_CODE > 0x000900
#	union rtai_lxrt_t tmp;
#	tmp = rtai_lxrt(srq, arg);
#	RETVAL = tmp.rt;
##endif
#    OUTPUT:
#	RETVAL

unsigned long
nam2num(name)
	char * name

void
num2nam(num, name)
	unsigned long  num
	char  * name
CODE:
	char tmp[8];
	num2nam(num, tmp);
	name = tmp;
OUTPUT:
	name

void *
rt_get_adr(name)
	int name

unsigned long
rt_get_name(adr)
	void * adr


RT_TASK *
rt_task_init(name, priority, stack_size, max_msg_size)
	int name
	int priority
	int stack_size
	int max_msg_size

int
rt_task_delete(task)
	RT_TASK * task

int
rt_task_yield()

int
rt_task_suspend(task)
	RT_TASK * task

int
rt_task_resume(task)
	RT_TASK * task

int
rt_task_make_periodic(task, start_time, period)
	RT_TASK * task
	RTIME     start_time
	RTIME     period

void
rt_task_wait_period()


void
rt_sleep(delay)
	RTIME delay

void
rt_sleep_until(time)
	RTIME time

int
start_rt_timer(period)
	int period

void
stop_rt_timer()

RTIME
rt_get_time()

RTIME
count2nano(count)
	RTIME count

RTIME
nano2count(nanos)
	RTIME nanos

SEM *
rt_sem_init(name, value)
	int name
	int value

int
rt_sem_delete(sem)
	SEM * sem

int
rt_sem_signal(sem)
	SEM * sem

int
rt_sem_wait(sem)
	SEM * sem

int
rt_sem_wait_if(sem)
	SEM * sem

int
rt_sem_wait_until(sem, time)
	SEM * sem
	RTIME time

int
rt_sem_wait_timed(sem, delay)
	SEM * sem
	RTIME delay


void
rt_busy_sleep(ns)
	int ns

RT_TASK *
rt_send(task, msg)
	RT_TASK      * task
	unsigned int   msg

RT_TASK *
rt_send_if(task, msg)
	RT_TASK      * task
	unsigned int   msg

RT_TASK *
rt_send_until(task, msg, time)
	RT_TASK      * task
	unsigned int   msg
	RTIME	       time

RT_TASK *
rt_send_timed(task, msg, delay)
	RT_TASK      * task
	unsigned int   msg
	RTIME	       delay
	
RT_TASK *
rt_receive(task, msg)
	RT_TASK      * task
	unsigned int * msg

RT_TASK *
rt_receive_if(task, msg)
	RT_TASK      * task
	unsigned int * msg

RT_TASK *
rt_receive_until(task, msg, time)
	RT_TASK      * task
	unsigned int * msg
	RTIME          time

RT_TASK *
rt_receive_timed(task, msg, delay)
	RT_TASK      * task
	unsigned int * msg
	RTIME          delay 
	
void
rt_set_oneshot_mode()

RT_TASK *
rt_agent()

