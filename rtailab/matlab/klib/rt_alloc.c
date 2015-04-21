/*
COPYRIGHT (C) 2003  Roberto Bucher (roberto.bucher@die.supsi.ch)

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include "linux/kernel.h"
#include "rt_mem_mgr.h"

#define size_t unsigned int

void *calloc(size_t nmemb, size_t size)
{
  unsigned int i;
  void *ptr;
  char *s;

  ptr=rt_malloc(nmemb*size);
  s=ptr;
  for(i=0;i<nmemb*size;i++) s[i]=0;
  return(ptr);
}

void free(void *ptr)
{
  if(ptr!=NULL) rt_free(ptr);
}

