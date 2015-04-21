
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

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "UnixToLVErr.c"

typedef struct {
	int32 dimSize;
	int32 fd[1];
	} TD1;
typedef TD1 **TD1Hdl;

typedef struct {
	int32 dimSize;
	int32 fd[1];
	} TD2;
typedef TD2 **TD2Hdl;

typedef struct {
	int32 dimSize;
	int32 fd[1];
	} TD3;
typedef TD3 **TD3Hdl;

CIN MgErr CINRun(int32 *retval, uInt32 *n, TD1Hdl LV_rfds, TD2Hdl LV_wfds, TD3Hdl LV_xfds, int32 *timeout, int32 *UnixErr, int32 *LVErr);

CIN MgErr CINRun(int32 *retval, uInt32 *n, TD1Hdl LV_rfds, TD2Hdl LV_wfds, TD3Hdl LV_xfds, int32 *timeout, int32 *UnixErr, int32 *LVErr) {
  
  int32 i;
  fd_set rfds, wfds, xfds;
  struct timeval tv;

  *UnixErr = 0;
  *LVErr = noErr;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  FD_ZERO(&xfds);

  /* put read fd in read_file_descriptor_set */
  i=0; 
  while ( i < (*LV_rfds)->dimSize ) {
    FD_SET( (int)((*LV_rfds)->fd[i]), &rfds ); 
    i++;
  }
  
  /* put write fd in write_file_descriptor_set */
  /* DbgPrintf( "# of write fd = %d", (*LV_wfds)->dimSize ); */
  i=0; 
  while ( i < (*LV_wfds)->dimSize ) {
    FD_SET( (int)((*LV_wfds)->fd[i]), &wfds ); 
    i++;
  }
  
  /* put exeption fd in exeption_file_descriptor_set */
  i=0; 
  while ( i < (*LV_xfds)->dimSize ) {
    FD_SET( (int)((*LV_xfds)->fd[i]), &wfds ); 
    i++;
  }
  
  /* no timeout: select will return immediately; waiting would block LabVIEW */
  *timeout = 0;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  /* select: wait for change of a fd in the sets */
  *retval = select( FD_SETSIZE, &rfds, &wfds, &xfds, &tv);

  /* error check */
  if ( *retval == -1 ) {
    *UnixErr = (int32) errno;
    *LVErr = (int32) UnixToLVErr();
    goto out;
  } else {
    /* find fd that triggered us and mark it */
    
    i=0; 
    while ( i < (*LV_rfds)->dimSize ) {
      int isset;
      isset = FD_ISSET( (*LV_rfds)->fd[i], &rfds );
      /* DbgPrintf( "ISSET(fd=%d, &rdfs) == %d", (*LV_rfds)->fd[i], isset ); */
      if ( isset ) {
	(*LV_rfds)->fd[i] = 1; 
      } else {
	(*LV_rfds)->fd[i] = 0; 
      }
      i++;
    }
    
    i=0; 
    while ( i < (*LV_wfds)->dimSize ) {
      if ( FD_ISSET( (*LV_wfds)->fd[i], &wfds) ) {
	(*LV_wfds)->fd[i] = 1; 
      } else {
	(*LV_wfds)->fd[i] = 0; 
      }
      i++;
    }
    
    i=0; 
    while ( i < (*LV_xfds)->dimSize ) {
      if ( FD_ISSET( (*LV_xfds)->fd[i], &xfds) ) {
	(*LV_xfds)->fd[i] = 1; 
      } else {
	(*LV_xfds)->fd[i] = 0; 
      }
      i++;
    }
  }
 out:
  return noErr;
}
