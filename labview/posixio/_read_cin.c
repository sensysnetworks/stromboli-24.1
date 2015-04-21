
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
#include <fcntl.h>
#include <errno.h>

#include "UnixToLVErr.c"

CIN MgErr CINRun(int32 *fd, LStrHandle buf, int32 *count, int32 *bytesread, int32 *UnixErr, int32 *LVErr);

CIN MgErr CINRun(int32 *fd, LStrHandle buf, int32 *count, int32 *bytesread, int32 *UnixErr, int32 *LVErr) {

  int32 cnt;
  uChar *str;
	
  *UnixErr = 0;
  *LVErr = noErr;
  *bytesread = 0;

  if ( *count < 0 ) { 
    goto out;
  }

  /* set LVstring to expected size */
  if ( *LVErr = NumericArrayResize( uB, 1L, (UHandle *)&buf, *count ) ) {
    goto out;
  } else {
    LStrLen(*buf) = *count;
  }
	
  /* read */
  if ( ( *bytesread = read( *fd, LStrBuf(*buf), *count ) ) < 0 ) {
    *UnixErr = (int32) errno;
    *LVErr = (int32) UnixToLVErr();
    goto out;
  } else {
    LStrLen(*buf) = *bytesread;
    /* adjust LVstring according to bytesread */
    if ( *LVErr = NumericArrayResize( uB, 1L, (UHandle *)&buf, *bytesread ) ) {
      goto out;
    }
  }
 out:
  return noErr;
}
