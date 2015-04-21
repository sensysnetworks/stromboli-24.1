
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#ifndef _TASK_HH_
#define _TASK_HH_

extern "C" {
#undef __KERNEL__
#define __CPP_KERNEL__
#include <sys/types.h>
#include <linux/types.h>
#include <rtai_lxrt_user.h> // This header file helps g++ a lot.
}

extern void Bootstrap(int this_task);

class Task {
   private:
	RT_TASK *task;

   protected:
	RTIME        period;
	bool         terminated;

	virtual void user_init()     {} // Pure virtuals.
	virtual void user_execute()  {} 
	virtual void user_cleanup()  {} 
        
   public:
	Task(int stk, int pri, RTIME per = 0);
	virtual ~Task() { rt_task_delete(task); rt_free(task); }

	void start(RTIME start = 0);

	void suspend() { rt_task_suspend(task); } // rt_schedule interface.
	void resume()  { rt_task_resume(task);  }
	void until_next_period() { rt_task_wait_period(); }
	void sleep(RTIME delay) { rt_sleep(delay); }
	
	void Terminate()    { terminated = true; }
	bool isTerminated() { return terminated; }

	friend void Bootstrap(int this_task);
};

#endif // _TASK_HH_
