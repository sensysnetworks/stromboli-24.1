
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#ifndef _HRT_HH_
#define _HRT_HH_

extern "C" {
	#include <sys/types.h>
	#include <sched.h>
	#include <rtai_lxrt_user.h> // This header file helps g++ a lot.
}

class Hrt {
   private:
	RT_TASK *task;

   protected:
	int heap, stack;
   public:
	Hrt(int stk, int heap, char *name);
	virtual ~Hrt() {
		rt_make_soft_real_time();
		rt_task_delete(task);
                rtai_print_to_screen("HRT mode ends\n");
	}
};

#endif // _HRT_HH_
