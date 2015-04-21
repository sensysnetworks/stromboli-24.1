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

CIN MgErr CINRun(LStrHandle Nam, uInt32 *Num, int32 *LVErr);
CIN MgErr CINProperties(int32 prop, void *data);

CIN MgErr CINProperties(int32 prop, void *data) { 
  switch (prop) { 
  case kCINIsReentrant: *(Bool32 *)data = TRUE; 
    return noErr;
  } 
  return mgNotSupported; 
}    	

CIN MgErr CINRun(LStrHandle Nam, uInt32 *Num, int32 *LVErr) {

  *LVErr = noErr; 
  /* set size string to 6 bytes */
  if ((*LVErr = NumericArrayResize( uB, 1L, (UHandle *)&Nam, sizeof(char[6])))) {
    goto out;
  } else {
    LStrLen(*Nam) = sizeof(char[6]);
    num2nam(*Num, LStrBuf(*Nam));
  }

out:
  return noErr;
}
