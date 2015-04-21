
/*
COPYRIGHT (C) 1999-2001  Thomas Leibner (leibner@t-online.de)

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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "UnixToLVErr.c"

CIN MgErr CINRun(int32 *fd, int32 *request, LStrHandle argp, int32 *argp_is_pointer, int32 *ioctl_ret, int32 *UnixErr, int32 *LVErr);

CIN MgErr CINRun(int32 *fd, int32 *request, LStrHandle argp, int32 *argp_is_pointer, int32 *ioctl_ret, int32 *UnixErr, int32 *LVErr) {


#define MAXLEN (1 << 14)

  int32 maxlen = MAXLEN;
  int32 no_pointer_value = 0;
  
  *UnixErr = 0;
  *LVErr = noErr;
  
  
  /* set LVstring to max size according to linux/ioctl.h */
  if ( ( *LVErr = NumericArrayResize( uB, 1L, (UHandle *)&argp, maxlen ) ) ) {
    goto out;
  } else {
    LStrLen(*argp) = maxlen;
  } 
  
  if (*argp_is_pointer != 0) {
    /* ioctl with *argp as pointer */
    if ( ( *ioctl_ret = ioctl( *fd, *request, LStrBuf(*argp) ) ) != 0 ) {
      *UnixErr = (int32) errno;
      *LVErr = (int32) UnixToLVErr();
    }
  } else {
    /* interpret *argp numerically, not as pointer */
    if ( (*LVErr = NumericArrayResize( uB, 1L, (UHandle *)&argp, sizeof(int32) ) ) ) {
      goto out;
    } else {
      LStrLen(*argp) = 4;
      /* copy first four bytes of LStrBuf(*argp) to *no_pointer_value */
      no_pointer_value = *((int32 *)LStrBuf(*argp));
      /* ioctl with *argp as number */
      if ( ( *ioctl_ret = ioctl( *fd, *request, no_pointer_value ) ) != 0 ) {
	*UnixErr = (int32) errno;
	*LVErr = (int32) UnixToLVErr();
      } else {
	/*
	  no_pointer_value++;
	  DbgPrintf("no_pointer_value=%d\n", no_pointer_value);
	*/
	/* copy value back to first four bytes of LStrBuf(*argp) */
	*((int32 *)LStrBuf(*argp)) = no_pointer_value;
      }
    }
  }
 out:
  return noErr;
}


