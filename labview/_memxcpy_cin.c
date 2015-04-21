/*
 * CIN source file 
 */

#include "extcode.h" 
#include <stdio.h>
#include <string.h>
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

CIN MgErr CINRun(uInt32 *DestAdr, uInt32 *SourceAdr, uInt32 *Length);
CIN MgErr CINProperties(int32 prop, void *data);
static int my_ds(void);

CIN MgErr CINProperties(int32 prop, void *data) { 
  switch (prop) { 
  case kCINIsReentrant: *(Bool32 *)data = TRUE; 
    return noErr;
  } 
  return mgNotSupported; 
}    
	
static int my_ds(void)
{
        int reg;
        __asm__("movl %%ds,%%eax " : "=a" (reg) : );
        return reg;
}                                                                                      

CIN MgErr CINRun(uInt32 *DestAdr, uInt32 *SourceAdr, uInt32 *Length) {
  int lsize;
  lsize = *Length/sizeof(int);
  //memcpy((void*)(*DestAdr), (void*)(*SourceAdr), *Length);
  // Which is this safe for hard realtime?
  _memxcpy((void*)(*DestAdr), (void*)(*SourceAdr), my_cs(), lsize); // from rtai_lxrt.h
  return noErr;
}
