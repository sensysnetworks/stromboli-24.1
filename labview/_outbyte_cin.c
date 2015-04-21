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
CIN MgErr CINRun(int32 *IOPort, uInt8 *Char);
CIN MgErr CINProperties(int32 prop, void *data);

CIN MgErr CINProperties(int32 prop, void *data) { 
  switch (prop) { 
  case kCINIsReentrant: *(Bool32 *)data = TRUE; 
    return noErr;
  } 
  return mgNotSupported; 
}    	
 
CIN MgErr CINRun(int32 *IOPort, uInt8 *Char) {

  outb((*Char) & 0xff, *IOPort);
  
  return noErr;
}
