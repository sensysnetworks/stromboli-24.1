#ifndef _ZDEFS_H_
#define _ZDEFS_H_
//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Trevor Woolven (trevw@zentropix.com)
// Original date:       Thu 15 Jul 1999
// Id:                  @(#)$Id: zdefs.h,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
// pqueues interface for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////

// -------------------------------< macros >-----------------------------------
#define REPORT(fmt, args...)  rt_printk("<Rep> " fmt ,##args)

#undef DBG
#ifdef ZDEBUG
#define DBG(fmt, args...)  rt_printk("<%s %d> " fmt, __FILE__, __LINE__ ,##args)
#else
#define DBG(fmt, args...)
#endif

#undef nDGB
#define nDBG(fmt, args...)


// ------------------------------< definitions >-------------------------------
typedef int STATUS;
typedef enum {FALSE, TRUE} BOOL;

#ifndef OK
#define OK	0
#endif
#ifndef ERROR
#define ERROR	-1
#endif

#define PASS	OK
#define FAIL	ERROR

// ---------------------------------< eof >------------------------------------

#endif  // _ZDEFS_H_
