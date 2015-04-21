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

#define size_t unsigned int

void exit(int status)
{
}

int strcmp(const char *s1, const char *s2)
{
  while (*s1 != '\0' && *s1 == *s2)
    {
      s1++;
      s2++;
    }

  return (*(unsigned char *) s1) - (*(unsigned char *) s2);
}

void *memcpy(void *dest, const void *src, size_t n)
{
  size_t i;
  char *s1, *s2;

  s1= (char*) src;
  s2= (char*) dest;

  for(i=0;i<n;i++) *s2++=*s1++;
  return(dest);
}
