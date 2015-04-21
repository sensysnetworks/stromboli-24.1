/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)

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

#ifndef _RTAI_DECLARE_H_
#define _RTAI_DECLARE_H_

#ifdef __KERNEL__
  #define KEEP_STATIC_INLINE
#endif

#ifdef KEEP_STATIC_INLINE
  #define DECLARE static inline
#else
  #ifdef KEEP_INLINE
    #define DECLARE inline
  #else
    #define DECLARE extern
  #endif
#endif

#endif // _RTAI_DECLARE_H_
