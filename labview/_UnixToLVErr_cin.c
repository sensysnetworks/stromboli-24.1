/*
 * CIN source file 
 */

#include "extcode.h"
#include <errno.h>
#include "UnixToLVErr.c"


CIN MgErr CINRun(int32 *UnixErr);
CIN MgErr CINProperties(int32 prop, void *data);

CIN MgErr CINProperties(int32 prop, void *data) { 
  switch (prop) { 
  case kCINIsReentrant: *(Bool32 *)data = TRUE; 
    return noErr;
  } 
  return mgNotSupported; 
} 
CIN MgErr CINRun(int32 *UnixErr) {

  /* ENTER YOUR CODE HERE */
  errno = *UnixErr;
  *UnixErr = UnixToLVErr();
  return noErr;
}
