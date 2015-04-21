/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)
extensions for user space modules are jointly copyrighted (2000) with:
                        Pierre Cloutier (pcloutier@poseidoncontrols.com),
                        Steve Papacharalambous (stevep@zentropix.com).

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


#ifndef _RTAI_LXRT_ASM_SUP_H_
#define _RTAI_LXRT_ASM_SUP_H_

DECLARE int my_cs(void)
{
#ifdef __i386__
	int reg; __asm__("movl %%cs,%%eax " : "=a" (reg) : ); return reg;
#endif
}

DECLARE void memxcpy(void *dst, int dseg, void *src, int sseg, int longs)
{
#ifdef __i386__
	// Generalised memxcpy
        __asm__ __volatile (\
        "cld; pushl %%ds; pushl %%es;\n\t"\
        "movl %%edx,%%es; movl %%eax,%%ds; rep; movsl;\n\t"\
        "popl %%es; popl %%ds;\n\t"\
//	:: "D" (dst),"S" (src),"ecx" (longs),"eax" (sseg),"edx" (dseg));
        : : "D" (dst),"S" (src),"c" (longs),"a" (sseg),"d" (dseg));
#endif
}

DECLARE void _memxcpy( void *dst, void *src, int srcseg, int longsize)
{
#ifdef __i386__
	__asm__ __volatile (\
	"cld;pushl %%ds;movl %%eax,%%ds;rep;movsl;popl %%ds;\n\t"\
//	:: "D" (dst), "S" (src), "ecx" (longsize), "eax" (srcseg));
	: : "D" (dst), "S" (src), "c" (longsize), "a" (srcseg));
#endif
} // Choice of registers limited by liblxrt gcc -fPIC option.

DECLARE union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg)
{
	static int Arg[12]; void *pt;
	lsize /= sizeof(int);
	if(my_cs() == __KERNEL_CS) {
		// With this we can reenter lxrt from a user space function.
		_memxcpy( &Arg, arg, __KERNEL_DS, lsize);
		pt = &Arg;
	} else pt = arg;

	return _rtai_lxrt((dynx << 28) | ((srq & 0xFFF) << 16) | lsize, pt);
}

#endif // _RTAI_LXRT_ASM_SUP_H_
