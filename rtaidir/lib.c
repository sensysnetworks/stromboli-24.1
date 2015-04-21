/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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



// One of the silly thing of 32 bits PPCs, no 64 bits result for 32 bits mul.
unsigned long long ullmul(unsigned long m0, unsigned long m1)
{
	unsigned long long res;

	__asm__ __volatile__ ("mulhwu %0, %1, %2"
	: "=r" (((unsigned long *)&res)[0]) : "%r" (m0), "r" (m1));
	((unsigned long *)&res)[1] = m0*m1;

	return res;

}

// One of the silly thing of 32 bits PPCs, no 64 by 32 bits divide.
unsigned long long ulldiv(unsigned long long ull, unsigned long uld, unsigned long *r)
{
	unsigned long long q, rf;
	unsigned long qh, rh, ql, qf;

	q = 0;
	rf = (unsigned long long)(0xFFFFFFFF - (qf = 0xFFFFFFFF / uld) * uld) + 1ULL;

	while (ull >= uld) {
		((unsigned long *)&q)[0] += (qh = ((unsigned long *)&ull)[0] / uld);
		rh = ((unsigned long *)&ull)[0] - qh * uld;
		q += rh * (unsigned long long)qf + (ql = ((unsigned long *)&ull)[1] / uld);
		ull = rh * rf + (((unsigned long *)&ull)[1] - ql * uld);
	}

	*r = ull;
	return q;
}

int imuldiv(int i, int mult, int div)
{
	unsigned long q, r;

	q = ulldiv(ullmul(i, mult), div, &r);

	return (r + r) > div ? q + 1 : q;
}

unsigned long long llimd(unsigned long long ull, unsigned long mult, unsigned long div)
{
	unsigned long long low;
	unsigned long q, r;

	low  = ullmul(((unsigned long *)&ull)[1], mult);	
	q = ulldiv( ullmul(((unsigned long *)&ull)[0], mult) + ((unsigned long *)&low)[0], div, (unsigned long *)&low);
	low = ulldiv(low, div, &r);
	((unsigned long *)&low)[0] += q;

	return (r + r) > div ? low + 1 : low;
}

