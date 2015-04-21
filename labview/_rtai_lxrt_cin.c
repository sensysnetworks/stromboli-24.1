
/*
COPYRIGHT (C) 1999-2002  Thomas Leibner (leibner@t-online.de)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.

*/
/*
 * CIN source file 
 */

#include "extcode.h"
#include "hosttype.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>

#include <sys/mman.h>

#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/io.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include "UnixToLVErr.c"

CIN MgErr CINRun(LStrHandle Rtai_lxrt_t, int16 *Dynx, int16 *Size, int32 *Srq, LStrHandle Arg, LStrHandle Buffer, int32 *BufPosInArg, int32 *UnixErr, int32 *LVErr);

CIN MgErr CINProperties(int32 prop, void *data);

CIN MgErr CINProperties(int32 prop, void *data) { 
  switch (prop) { 
  case kCINIsReentrant: *(Bool32 *)data = TRUE; 
    return noErr;
  } 
  return mgNotSupported; 
}    	

CIN MgErr CINRun(LStrHandle Rtai_lxrt_t, int16 *Dynx, int16 *Size, int32 *Srq, LStrHandle Arg, LStrHandle Buffer, int32 *BufPosInArg, int32 *UnixErr, int32 *LVErr) {

  int  DEBUG=0;  
  union rtai_lxrt_t retval;
  int *iArg=NULL;
  char lbuf[LStrLen(*Buffer)+1]; // dynamically allocate on Stack; +1 if it is zero

  *UnixErr = 0;
  *LVErr = noErr;
  DEBUG=LStrLen(*Rtai_lxrt_t)+1;
  if (DEBUG>2) DEBUG=0;
  if (DEBUG) {  
    DbgPrintf(NULL); /* Close window */
    DbgPrintf("Debuglevel:%d, calling lxrt:%d\n", DEBUG, DEBUG-1);
  }
  // DbgPrintf("DEBUG=%d\n", DEBUG);

  /* set size of return string */
  
  if ((*LVErr = NumericArrayResize( uB, 1L, (UHandle *)&Rtai_lxrt_t, sizeof(union rtai_lxrt_t)))) {
    goto out;
  } else {
    LStrLen(*Rtai_lxrt_t) = sizeof(union rtai_lxrt_t);
  }
  
  
  /* set size of argument string according to int[12], see rtai_rtai.h  */
  
  if ((*LVErr = NumericArrayResize( uB, 1L, (UHandle *)&Arg, sizeof(int[12])))) {
    goto out;
  } else {
    LStrLen(*Arg) = sizeof(int[12]);
  }
  
  iArg=(int*)LStrBuf(*Arg);
  
  /* do we have to insert a Buffer into the arguments? */
  if ((*BufPosInArg > 0) && (*BufPosInArg < 12)) {
    if (DEBUG) DbgPrintf("Handling BufferAdr to inserted at PosInArg=%d with BufferLen=%d\n", *BufPosInArg, LStrLen(*Buffer));
    /* copy Buffer to local mem on stack */
    if (DEBUG) DbgPrintf("Doing memcpy(0x%x, 0x%x, %d)", lbuf, LStrBuf(*Buffer), LStrLen(*Buffer));
    memcpy(lbuf, LStrBuf(*Buffer), LStrLen(*Buffer));
    /* and insert &lbuf into *Arg */
    ((int*)LStrBuf(*Arg))[*BufPosInArg] = (int)lbuf;
  }
  
  //*Size = (int16)LStrLen(*Arg);
  if (DEBUG) {  
    /* print arguments */
    DbgPrintf("(int) Arg[0...3]: 0x%x 0x%x 0x%x 0x%x\n", iArg[0],iArg[1],iArg[2],iArg[3]);
    DbgPrintf("(int) Arg[4...7]: 0x%x 0x%x 0x%x 0x%x\n", iArg[4],iArg[5],iArg[6],iArg[7]);
    DbgPrintf("(int) Arg[8..11]: 0x%x 0x%x 0x%x 0x%x\n", iArg[8],iArg[9],iArg[10],iArg[11]);
        
    if ((*BufPosInArg > 0) && (*BufPosInArg < 12)) {
      int i;
      DbgPrintf("Buffer string from Arg[%d]:", *BufPosInArg);
      for (i=0; i<LStrLen(*Buffer); i++) {
	DbgPrintf("0x%02x", (int) (((char*)(iArg[*BufPosInArg]))[i]));
      }
    }
  }
  if ((DEBUG==0) || (DEBUG==2)) {
    /* rtai_lxrt call */
    if (DEBUG==2) {
      DbgPrintf("Calling rtai_lxrt(Dynx=%d, Size=%d, Srq=%d, int Arg[0..11])\n",(int) *Dynx, (int) *Size, (int) *Srq);  
    }
    retval = rtai_lxrt( *Dynx, *Size, *Srq, LStrBuf(*Arg));
  }
  /* copy retval back to first eight bytes of LStrBuf(*Rtai_lxrt_t) */
  if (DEBUG==2) {
    DbgPrintf("Retval[0..1]: %x %x\n", retval.i[LOW], retval.i[HIGH]);
  }
  *((int32 *)LStrBuf(*Rtai_lxrt_t)) = retval.i[LOW];
  *((int32 *)LStrBuf(*Rtai_lxrt_t)+1) = retval.i[HIGH];
  
  /* if Buffer used copy back from Stack */
  if ((*BufPosInArg > 0) && (*BufPosInArg < 12)) {
    int i;
    if (DEBUG) DbgPrintf("Doing memcpy(0x%x, 0x%x, %d)", LStrBuf(*Buffer), lbuf, LStrLen(*Buffer));
    memcpy(LStrBuf(*Buffer), lbuf, LStrLen(*Buffer));
    if (DEBUG) {
      DbgPrintf("Buffer contents now:");
      for (i=0; i<LStrLen(*Buffer); i++) {
	DbgPrintf("0x%02x", (int) (((char*)LStrBuf(*Buffer))[i]));
      }
    }
  }
  
  *UnixErr = (int32) 0; //errno;
  *LVErr = (int32) 0; //UnixToLVErr();
 out:
  return noErr;
}
