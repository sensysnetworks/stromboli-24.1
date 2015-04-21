/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#define TICK_PERIOD  1000  // in us
#define WARMUP       10

#define NROWS      300
#define NCOLS      300
#define MAXBUFREC  1000
#define RECSIZE    5000  // bytes, internally transformed in multiple of int

#define SHMNAM       "MIRSHM"

#define SYNFIF       0
#define SYNDEV       "/dev/rtf0"
#define SYNFIF_SIZE  1000  // in bytes
#define SHMFIF       1
#define SHMDEV       "/dev/rtf1"
#define SHMFIF_SIZE  1000  // in bytes

#define DELAY        10    // in ms, less then 10 causes no delay (HZ = 100)

#define LOGFILE       "logfile"
#define LOGFILE_SIZE  100000000  // in bytes

#define RTYPE float

struct shm_control_block { int run;
			   int maxrec, recsize, nrec, nrecw, frec, lrec, buf_base_ofst;
			   int nr, nc, nrb, nmeb, slope_to_pos_mat_ofst; };

#define RNUM  3.141592  // a number to do something
#define REPORT_CNT 500  // macro to control the info interval, in intr counts

struct exec_info { int cpuid, cnt, avrj, maxj, res, last; long long exctim; };

#ifdef __KERNEL__
static __inline__ int atomic_inc_and_test_greater_zero(volatile atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK "incl %0; setg %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter));
	return c; /* can be only 0 or 1 */
}
#endif
