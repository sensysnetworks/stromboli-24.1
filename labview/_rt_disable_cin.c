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

CIN MgErr CINRun(int *Name, int32 *RetVal, int32 *UnixErr, int32 *LVErr);
CIN MgErr CINProperties(int32 prop, void *data);

CIN MgErr CINProperties(int32 prop, void *data) { 
	switch (prop) { 
		case kCINIsReentrant: *(Bool32 *)data = TRUE; 
		return noErr;
		} 
	return mgNotSupported; 
	}    	


CIN MgErr CINRun(int32 *Name, int32 *RetVal, int32 *UnixErr, int32 *LVErr) {
  RT_TASK *adr;

  rt_allow_nonroot_hrt();
  //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  if (iopl(0)) {
    DbgPrintf("iopl(0) failed/n");
    *UnixErr = (int32)1;
    *LVErr = (int32)1;
    goto out;
  }  
  
  stop_rt_timer();
  
  rt_make_soft_real_time();
  
  if (munlockall()) {
    DbgPrintf("munlockall() failed/n");
    *UnixErr = (int32)1;
    *LVErr = (int32)1;
    goto out;
  }  
  
  if ((adr = rt_get_adr(*Name)) != 0) {
    rt_task_delete(adr);
  } else {
    DbgPrintf("rt_task_delete(%d) failed/n", *Name);
    *UnixErr = (int32)1;
    *LVErr = (int32)1;
    goto out;
  }
   
  *UnixErr = (int32) 0; //errno;
  *LVErr = (int32) 0; //UnixToLVErr();
 out:	
  return noErr;
}
                                          
