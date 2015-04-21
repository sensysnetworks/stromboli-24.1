/*
 * CIN source file 
 */

#include "extcode.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include "UnixToLVErr.c"  

CIN MgErr CINRun(int32 *Name, int32 *Hrt, uInt32 *StackSize, int32 *RetVal, int32 *UnixErr, int32 *LVErr);
CIN MgErr CINProperties(int32 prop, void *data);

CIN MgErr CINProperties(int32 prop, void *data) { 
	switch (prop) { 
		case kCINIsReentrant: *(Bool32 *)data = TRUE; 
		return noErr;
		} 
	return mgNotSupported; 
	}    	


CIN MgErr CINRun(int32 *Name,int32 *Hrt, uInt32 *StackSize, int32 *RetVal, int32 *UnixErr, int32 *LVErr) {

  RT_TASK *lvtask;

  *RetVal = 0;
  *UnixErr = 0;
  *LVErr = 0;

  rt_allow_nonroot_hrt();
  //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  if (iopl(3)) {
    DbgPrintf("iopl(3) failed/n");
    *UnixErr = (int32)1;
    *LVErr = (int32)1;
    goto out;
  }  
  
  if (!(lvtask = rt_task_init_schmod(*Name, 1, 0, 0, SCHED_FIFO, 0xF))) {
    DbgPrintf("CANNOT INIT TASK 0x%x\n", *Name);
    *UnixErr = (int32) 1;
    *LVErr = (int32) 1;
    goto out;
  } else {
    *RetVal = (int32)lvtask;
  }
  rt_set_oneshot_mode();
  start_rt_timer(0);
  
#define STACKDEFAULT 256*1024

  if (*StackSize < STACKDEFAULT) { 
    *StackSize = STACKDEFAULT;
  }

  rt_grow_and_lock_stack((*StackSize)); 
  
  if (*Hrt == 1) {
    //DbgPrintf("Going hard realtime...");
    rt_make_hard_real_time();
  }
  
  
  *UnixErr = (int32) 0; //errno;
  *LVErr = (int32) 0; //UnixToLVErr();
 out:	
  return noErr;
}
                                          
