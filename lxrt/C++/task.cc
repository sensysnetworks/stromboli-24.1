
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#include "task.hh"

Task::Task(int stk, int pri, RTIME per)
{
	int fp ;
	void (*sig)(void);

	task = rt_alloc_dynamic_task();
	if(!task) rt_printk("rt_allocate_dyn_task failed\n");

	if(rt_task_init( task, Bootstrap, int(this), stk, pri, fp=1, sig=0))
		rt_printk("rt_task_init failed\n");

	period = per;
	terminated = false;
}

void Task::start(RTIME start)
{

	if (period != 0) {
		if (rt_task_make_periodic(task, (start) ? start : rt_get_time(), period)) {
			rt_printk("rt_task_make_periodic failed\n");	
			}
	} else 
		rt_task_resume(task);
}

void Bootstrap(int task)
{
	Task &t = *((Task *)task);
	t.user_init();
	t.user_execute();
	t.user_cleanup(); 
	rt_task_suspend(t.task);
}
