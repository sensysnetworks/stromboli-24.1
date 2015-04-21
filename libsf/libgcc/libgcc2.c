/* Functions for RTAI softfloat support hacked from GCC sources 
	Copyright (c) 2003, Thomas Gleixner, <tglx@linutronix.de)
*/
/* More subroutines needed by GCC output code on some machines.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001  Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file into combinations with other programs,
and to distribute those combinations without any restriction coming
from the use of this file.  (The General Public License restrictions
do apply in other respects; for example, they cover modification of
the file, and distribution when not linked into a combine
executable.)

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* It is incorrect to include config.h here, because this file is being
   compiled for the target, and hence definitions concerning only the host
   do not apply.  */

#include "arch.h"
#include "defaults.h"

/* Don't use `fancy_abort' here even if config.h says to use it.  */
#ifdef abort
#undef abort
#endif

#include "libgcc2.h"

#if defined (L_negdi2) || defined (L_divdi3) || defined (L_moddi3)
#if defined (L_divdi3) || defined (L_moddi3)
static inline
#endif
DWtype
__negdi2 (DWtype u)
{
  DWunion w;
  DWunion uu;

  uu.ll = u;

  w.s.low = -uu.s.low;
  w.s.high = -uu.s.high - ((UWtype) w.s.low > 0);

  return w.ll;
}
#endif

#ifdef L_addvsi3
Wtype
__addvsi3 (Wtype a, Wtype b)
{
  Wtype w;

  w = a + b;

  if (b >= 0 ? w < a : w > a)
    abort ();

  return w;
} 
#endif

#ifdef L_addvdi3
DWtype
__addvdi3 (DWtype a, DWtype b)
{
  DWtype w;

  w = a + b;

  if (b >= 0 ? w < a : w > a)
    abort ();

  return w;
}
#endif

#ifdef L_subvsi3
Wtype
__subvsi3 (Wtype a, Wtype b)
{
#ifdef L_addvsi3
  return __addvsi3 (a, (-b));
#else
  DWtype w;

  w = a - b;

  if (b >= 0 ? w > a : w < a)
    abort ();

  return w;
#endif
}
#endif

#ifdef L_subvdi3
DWtype
__subvdi3 (DWtype a, DWtype b)
{
#ifdef L_addvdi3
  return (a, (-b));
#else
  DWtype w;

  w = a - b;

  if (b >= 0 ? w > a : w < a)
    abort ();

  return w;
#endif
}
#endif

#ifdef L_mulvsi3
Wtype
__mulvsi3 (Wtype a, Wtype b)
{
  DWtype w;

  w = a * b;

  if (((a >= 0) == (b >= 0)) ? w < 0 : w > 0)
    abort ();

  return w;
}
#endif

#ifdef L_negvsi2
Wtype
__negvsi2 (Wtype a)
{
   Wtype w;

   w  = -a;

  if (a >= 0 ? w > 0 : w < 0)
    abort ();

   return w;
}
#endif

#ifdef L_negvdi2
DWtype
__negvdi2 (DWtype a)
{
   DWtype w;

   w  = -a;

  if (a >= 0 ? w > 0 : w < 0)
    abort ();

   return w;
}
#endif

#ifdef L_absvsi2
Wtype
__absvsi2 (Wtype a)
{
   Wtype w = a;

   if (a < 0)
#ifdef L_negvsi2
     w = __negvsi2 (a);
#else
     w = -a;

   if (w < 0)
     abort ();
#endif

   return w;
}
#endif

#ifdef L_absvdi2
DWtype
__absvdi2 (DWtype a)
{
   DWtype w = a;

   if (a < 0)
#ifdef L_negvsi2
     w = __negvsi2 (a);
#else
     w = -a;

   if (w < 0)
     abort ();
#endif

   return w;
}
#endif

#ifdef L_mulvdi3
DWtype
__mulvdi3 (DWtype u, DWtype v)
{
   DWtype w;

  w = u * v;

  if (((u >= 0) == (v >= 0)) ? w < 0 : w > 0)
    abort ();

  return w;
}
#endif


/* Unless shift functions are defined whith full ANSI prototypes,
   parameter b will be promoted to int if word_type is smaller than an int.  */
#ifdef L_lshrdi3
DWtype
__lshrdi3 (DWtype u, word_type b)
{
  DWunion w;
  word_type bm;
  DWunion uu;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (Wtype) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.high = 0;
      w.s.low = (UWtype) uu.s.high >> -bm;
    }
  else
    {
      UWtype carries = (UWtype) uu.s.high << bm;

      w.s.high = (UWtype) uu.s.high >> b;
      w.s.low = ((UWtype) uu.s.low >> b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ashldi3
DWtype
__ashldi3 (DWtype u, word_type b)
{
  DWunion w;
  word_type bm;
  DWunion uu;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (Wtype) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.low = 0;
      w.s.high = (UWtype) uu.s.low << -bm;
    }
  else
    {
      UWtype carries = (UWtype) uu.s.low >> bm;

      w.s.low = (UWtype) uu.s.low << b;
      w.s.high = ((UWtype) uu.s.high << b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ashrdi3
DWtype
__ashrdi3 (DWtype u, word_type b)
{
  DWunion w;
  word_type bm;
  DWunion uu;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (Wtype) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      /* w.s.high = 1..1 or 0..0 */
      w.s.high = uu.s.high >> (sizeof (Wtype) * BITS_PER_UNIT - 1);
      w.s.low = uu.s.high >> -bm;
    }
  else
    {
      UWtype carries = (UWtype) uu.s.high << bm;

      w.s.high = uu.s.high >> b;
      w.s.low = ((UWtype) uu.s.low >> b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ffsdi2
DWtype
__ffsdi2 (DWtype u)
{
  DWunion uu;
  UWtype word, count, add;

  uu.ll = u;
  if (uu.s.low != 0)
    word = uu.s.low, add = 0;
  else if (uu.s.high != 0)
    word = uu.s.high, add = BITS_PER_UNIT * sizeof (Wtype);
  else
    return 0;

  count_trailing_zeros (count, word);
  return count + add + 1;
}
#endif

#ifdef L_muldi3
DWtype
__muldi3 (DWtype u, DWtype v)
{
  DWunion w;
  DWunion uu, vv;

  uu.ll = u,
  vv.ll = v;

  w.ll = __umulsidi3 (uu.s.low, vv.s.low);
  w.s.high += ((UWtype) uu.s.low * (UWtype) vv.s.high
	       + (UWtype) uu.s.high * (UWtype) vv.s.low);

  return w.ll;
}
#endif

#ifdef L_udiv_w_sdiv
#if defined (sdiv_qrnnd)
UWtype
__udiv_w_sdiv (UWtype *rp, UWtype a1, UWtype a0, UWtype d)
{
  UWtype q, r;
  UWtype c0, c1, b1;

  if ((Wtype) d >= 0)
    {
      if (a1 < d - a1 - (a0 >> (W_TYPE_SIZE - 1)))
	{
	  /* dividend, divisor, and quotient are nonnegative */
	  sdiv_qrnnd (q, r, a1, a0, d);
	}
      else
	{
	  /* Compute c1*2^32 + c0 = a1*2^32 + a0 - 2^31*d */
	  sub_ddmmss (c1, c0, a1, a0, d >> 1, d << (W_TYPE_SIZE - 1));
	  /* Divide (c1*2^32 + c0) by d */
	  sdiv_qrnnd (q, r, c1, c0, d);
	  /* Add 2^31 to quotient */
	  q += (UWtype) 1 << (W_TYPE_SIZE - 1);
	}
    }
  else
    {
      b1 = d >> 1;			/* d/2, between 2^30 and 2^31 - 1 */
      c1 = a1 >> 1;			/* A/2 */
      c0 = (a1 << (W_TYPE_SIZE - 1)) + (a0 >> 1);

      if (a1 < b1)			/* A < 2^32*b1, so A/2 < 2^31*b1 */
	{
	  sdiv_qrnnd (q, r, c1, c0, b1); /* (A/2) / (d/2) */

	  r = 2*r + (a0 & 1);		/* Remainder from A/(2*b1) */
	  if ((d & 1) != 0)
	    {
	      if (r >= q)
		r = r - q;
	      else if (q - r <= d)
		{
		  r = r - q + d;
		  q--;
		}
	      else
		{
		  r = r - q + 2*d;
		  q -= 2;
		}
	    }
	}
      else if (c1 < b1)			/* So 2^31 <= (A/2)/b1 < 2^32 */
	{
	  c1 = (b1 - 1) - c1;
	  c0 = ~c0;			/* logical NOT */

	  sdiv_qrnnd (q, r, c1, c0, b1); /* (A/2) / (d/2) */

	  q = ~q;			/* (A/2)/b1 */
	  r = (b1 - 1) - r;

	  r = 2*r + (a0 & 1);		/* A/(2*b1) */

	  if ((d & 1) != 0)
	    {
	      if (r >= q)
		r = r - q;
	      else if (q - r <= d)
		{
		  r = r - q + d;
		  q--;
		}
	      else
		{
		  r = r - q + 2*d;
		  q -= 2;
		}
	    }
	}
      else				/* Implies c1 = b1 */
	{				/* Hence a1 = d - 1 = 2*b1 - 1 */
	  if (a0 >= -d)
	    {
	      q = -1;
	      r = a0 + d;
	    }
	  else
	    {
	      q = -2;
	      r = a0 + 2*d;
	    }
	}
    }

  *rp = r;
  return q;
}
#else
/* If sdiv_qrnnd doesn't exist, define dummy __udiv_w_sdiv.  */
UWtype
__udiv_w_sdiv (UWtype *rp __attribute__ ((__unused__)),
	       UWtype a1 __attribute__ ((__unused__)),
	       UWtype a0 __attribute__ ((__unused__)),
	       UWtype d __attribute__ ((__unused__)))
{
  return 0;
}
#endif
#endif

#if (defined (L_udivdi3) || defined (L_divdi3) || \
     defined (L_umoddi3) || defined (L_moddi3))
#define L_udivmoddi4
#endif

#ifdef L_clz
const UQItype __clz_tab[] =
{
  0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};
#endif

#ifdef L_udivmoddi4

#if (defined (L_udivdi3) || defined (L_divdi3) || \
     defined (L_umoddi3) || defined (L_moddi3))
static inline
#endif
UDWtype
__udivmoddi4 (UDWtype n, UDWtype d, UDWtype *rp)
{
  DWunion ww;
  DWunion nn, dd;
  DWunion rr;
  UWtype d0, d1, n0, n1, n2;
  UWtype q0, q1;
  UWtype b, bm;

  nn.ll = n;
  dd.ll = d;

  d0 = dd.s.low;
  d1 = dd.s.high;
  n0 = nn.s.low;
  n1 = nn.s.high;

#if !UDIV_NEEDS_NORMALIZATION
  if (d1 == 0)
    {
      if (d0 > n1)
	{
	  /* 0q = nn / 0D */

	  udiv_qrnnd (q0, n0, n1, n0, d0);
	  q1 = 0;

	  /* Remainder in n0.  */
	}
      else
	{
	  /* qq = NN / 0d */

	  if (d0 == 0)
	    d0 = 1 / d0;	/* Divide intentionally by zero.  */

	  udiv_qrnnd (q1, n1, 0, n1, d0);
	  udiv_qrnnd (q0, n0, n1, n0, d0);

	  /* Remainder in n0.  */
	}

      if (rp != 0)
	{
	  rr.s.low = n0;
	  rr.s.high = 0;
	  *rp = rr.ll;
	}
    }

#else /* UDIV_NEEDS_NORMALIZATION */

  if (d1 == 0)
    {
      if (d0 > n1)
	{
	  /* 0q = nn / 0D */

	  count_leading_zeros (bm, d0);

	  if (bm != 0)
	    {
	      /* Normalize, i.e. make the most significant bit of the
		 denominator set.  */

	      d0 = d0 << bm;
	      n1 = (n1 << bm) | (n0 >> (W_TYPE_SIZE - bm));
	      n0 = n0 << bm;
	    }

	  udiv_qrnnd (q0, n0, n1, n0, d0);
	  q1 = 0;

	  /* Remainder in n0 >> bm.  */
	}
      else
	{
	  /* qq = NN / 0d */

	  if (d0 == 0)
	    d0 = 1 / d0;	/* Divide intentionally by zero.  */

	  count_leading_zeros (bm, d0);

	  if (bm == 0)
	    {
	      /* From (n1 >= d0) /\ (the most significant bit of d0 is set),
		 conclude (the most significant bit of n1 is set) /\ (the
		 leading quotient digit q1 = 1).

		 This special case is necessary, not an optimization.
		 (Shifts counts of W_TYPE_SIZE are undefined.)  */

	      n1 -= d0;
	      q1 = 1;
	    }
	  else
	    {
	      /* Normalize.  */

	      b = W_TYPE_SIZE - bm;

	      d0 = d0 << bm;
	      n2 = n1 >> b;
	      n1 = (n1 << bm) | (n0 >> b);
	      n0 = n0 << bm;

	      udiv_qrnnd (q1, n1, n2, n1, d0);
	    }

	  /* n1 != d0...  */

	  udiv_qrnnd (q0, n0, n1, n0, d0);

	  /* Remainder in n0 >> bm.  */
	}

      if (rp != 0)
	{
	  rr.s.low = n0 >> bm;
	  rr.s.high = 0;
	  *rp = rr.ll;
	}
    }
#endif /* UDIV_NEEDS_NORMALIZATION */

  else
    {
      if (d1 > n1)
	{
	  /* 00 = nn / DD */

	  q0 = 0;
	  q1 = 0;

	  /* Remainder in n1n0.  */
	  if (rp != 0)
	    {
	      rr.s.low = n0;
	      rr.s.high = n1;
	      *rp = rr.ll;
	    }
	}
      else
	{
	  /* 0q = NN / dd */

	  count_leading_zeros (bm, d1);
	  if (bm == 0)
	    {
	      /* From (n1 >= d1) /\ (the most significant bit of d1 is set),
		 conclude (the most significant bit of n1 is set) /\ (the
		 quotient digit q0 = 0 or 1).

		 This special case is necessary, not an optimization.  */

	      /* The condition on the next line takes advantage of that
		 n1 >= d1 (true due to program flow).  */
	      if (n1 > d1 || n0 >= d0)
		{
		  q0 = 1;
		  sub_ddmmss (n1, n0, n1, n0, d1, d0);
		}
	      else
		q0 = 0;

	      q1 = 0;

	      if (rp != 0)
		{
		  rr.s.low = n0;
		  rr.s.high = n1;
		  *rp = rr.ll;
		}
	    }
	  else
	    {
	      UWtype m1, m0;
	      /* Normalize.  */

	      b = W_TYPE_SIZE - bm;

	      d1 = (d1 << bm) | (d0 >> b);
	      d0 = d0 << bm;
	      n2 = n1 >> b;
	      n1 = (n1 << bm) | (n0 >> b);
	      n0 = n0 << bm;

	      udiv_qrnnd (q0, n1, n2, n1, d1);
	      umul_ppmm (m1, m0, q0, d0);

	      if (m1 > n1 || (m1 == n1 && m0 > n0))
		{
		  q0--;
		  sub_ddmmss (m1, m0, m1, m0, d1, d0);
		}

	      q1 = 0;

	      /* Remainder in (n1n0 - m1m0) >> bm.  */
	      if (rp != 0)
		{
		  sub_ddmmss (n1, n0, n1, n0, m1, m0);
		  rr.s.low = (n1 << b) | (n0 >> bm);
		  rr.s.high = n1 >> bm;
		  *rp = rr.ll;
		}
	    }
	}
    }

  ww.s.low = q0;
  ww.s.high = q1;
  return ww.ll;
}
#endif

#ifdef L_divdi3
DWtype
__divdi3 (DWtype u, DWtype v)
{
  word_type c = 0;
  DWunion uu, vv;
  DWtype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c,
    uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    c = ~c,
    vv.ll = __negdi2 (vv.ll);

  w = __udivmoddi4 (uu.ll, vv.ll, (UDWtype *) 0);
  if (c)
    w = __negdi2 (w);

  return w;
}
#endif

#ifdef L_moddi3
DWtype
__moddi3 (DWtype u, DWtype v)
{
  word_type c = 0;
  DWunion uu, vv;
  DWtype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c,
    uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    vv.ll = __negdi2 (vv.ll);

  (void) __udivmoddi4 (uu.ll, vv.ll, &w);
  if (c)
    w = __negdi2 (w);

  return w;
}
#endif

#ifdef L_umoddi3
UDWtype
__umoddi3 (UDWtype u, UDWtype v)
{
  UDWtype w;

  (void) __udivmoddi4 (u, v, &w);

  return w;
}
#endif

#ifdef L_udivdi3
UDWtype
__udivdi3 (UDWtype n, UDWtype d)
{
  return __udivmoddi4 (n, d, (UDWtype *) 0);
}
#endif

#ifdef L_cmpdi2
word_type
__cmpdi2 (DWtype a, DWtype b)
{
  DWunion au, bu;

  au.ll = a, bu.ll = b;

  if (au.s.high < bu.s.high)
    return 0;
  else if (au.s.high > bu.s.high)
    return 2;
  if ((UWtype) au.s.low < (UWtype) bu.s.low)
    return 0;
  else if ((UWtype) au.s.low > (UWtype) bu.s.low)
    return 2;
  return 1;
}
#endif

#ifdef L_ucmpdi2
word_type
__ucmpdi2 (DWtype a, DWtype b)
{
  DWunion au, bu;

  au.ll = a, bu.ll = b;

  if ((UWtype) au.s.high < (UWtype) bu.s.high)
    return 0;
  else if ((UWtype) au.s.high > (UWtype) bu.s.high)
    return 2;
  if ((UWtype) au.s.low < (UWtype) bu.s.low)
    return 0;
  else if ((UWtype) au.s.low > (UWtype) bu.s.low)
    return 2;
  return 1;
}
#endif

#if defined(L_fixunstfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunstfDI (TFtype a)
{
  TFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the TFtype, leaving the low part as flonum.  */
  a -= (TFtype)v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#if defined(L_fixtfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128)
DWtype
__fixtfdi (TFtype a)
{
  if (a < 0)
    return - __fixunstfDI (-a);
  return __fixunstfDI (a);
}
#endif

#if defined(L_fixunsxfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunsxfDI (XFtype a)
{
  XFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the XFtype, leaving the low part as flonum.  */
  a -= (XFtype)v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#if defined(L_fixxfdi) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96)
DWtype
__fixxfdi (XFtype a)
{
  if (a < 0)
    return - __fixunsxfDI (-a);
  return __fixunsxfDI (a);
}
#endif

#ifdef L_fixunsdfdi
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunsdfDI (DFtype a)
{
  DFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the DFtype, leaving the low part as flonum.  */
  a -= (DFtype)v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#ifdef L_fixdfdi
DWtype
__fixdfdi (DFtype a)
{
  if (a < 0)
    return - __fixunsdfDI (-a);
  return __fixunsdfDI (a);
}
#endif

#ifdef L_fixunssfdi
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DWtype
__fixunssfDI (SFtype original_a)
{
  /* Convert the SFtype to a DFtype, because that is surely not going
     to lose any bits.  Some day someone else can write a faster version
     that avoids converting to DFtype, and verify it really works right.  */
  DFtype a = original_a;
  DFtype b;
  UDWtype v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DWtype!),
     and shift it into the high word.  */
  v = (UWtype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the DFtype, leaving the low part as flonum.  */
  a -= (DFtype) v;
  /* Convert that to fixed (but not to DWtype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (UWtype) (- a);
  else
    v += (UWtype) a;
  return v;
}
#endif

#ifdef L_fixsfdi
DWtype
__fixsfdi (SFtype a)
{
  if (a < 0)
    return - __fixunssfDI (-a);
  return __fixunssfDI (a);
}
#endif

#if defined(L_floatdixf) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

XFtype
__floatdixf (DWtype u)
{
  XFtype d;

  d = (Wtype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}
#endif

#if defined(L_floatditf) && (LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 128)
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

TFtype
__floatditf (DWtype u)
{
  TFtype d;

  d = (Wtype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}
#endif

#ifdef L_floatdidf
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)

DFtype
__floatdidf (DWtype u)
{
  DFtype d;

  d = (Wtype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}
#endif

#ifdef L_floatdisf
#define WORD_SIZE (sizeof (Wtype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDWtype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDWtype) 1) << WORD_SIZE)
#define DI_SIZE (sizeof (DWtype) * BITS_PER_UNIT)

/* Define codes for all the float formats that we know of.  Note
   that this is copied from real.h.  */
   
#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define VAX_FLOAT_FORMAT 2
#define IBM_FLOAT_FORMAT 3

/* Default to IEEE float if not specified.  Nearly all machines use it.  */
#ifndef HOST_FLOAT_FORMAT
#define	HOST_FLOAT_FORMAT	IEEE_FLOAT_FORMAT
#endif

#if HOST_FLOAT_FORMAT == IEEE_FLOAT_FORMAT
#define DF_SIZE 53
#define SF_SIZE 24
#endif

#if HOST_FLOAT_FORMAT == IBM_FLOAT_FORMAT
#define DF_SIZE 56
#define SF_SIZE 24
#endif

#if HOST_FLOAT_FORMAT == VAX_FLOAT_FORMAT
#define DF_SIZE 56
#define SF_SIZE 24
#endif

SFtype
__floatdisf (DWtype u)
{
  /* Do the calculation in DFmode
     so that we don't lose any of the precision of the high word
     while multiplying it.  */
  DFtype f;

  /* Protect against double-rounding error.
     Represent any low-order bits, that might be truncated in DFmode,
     by a bit that won't be lost.  The bit can go in anywhere below the
     rounding position of the SFmode.  A fixed mask and bit position
     handles all usual configurations.  It doesn't handle the case
     of 128-bit DImode, however.  */
  if (DF_SIZE < DI_SIZE
      && DF_SIZE > (DI_SIZE - DF_SIZE + SF_SIZE))
    {
#define REP_BIT ((UDWtype) 1 << (DI_SIZE - DF_SIZE))
      if (! (- ((DWtype) 1 << DF_SIZE) < u
	     && u < ((DWtype) 1 << DF_SIZE)))
	{
	  if ((UDWtype) u & (REP_BIT - 1))
	    u |= REP_BIT;
	}
    }
  f = (Wtype) (u >> WORD_SIZE);
  f *= HIGH_HALFWORD_COEFF;
  f *= HIGH_HALFWORD_COEFF;
  f += (UWtype) (u & (HIGH_WORD_COEFF - 1));

  return (SFtype) f;
}
#endif

#if defined(L_fixunsxfsi) && LIBGCC2_LONG_DOUBLE_TYPE_SIZE == 96
/* Reenable the normal types, in case limits.h needs them.  */
#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double
#undef MIN
#undef MAX
#include <limits.h>

UWtype
__fixunsxfSI (XFtype a)
{
  if (a >= - (DFtype) Wtype_MIN)
    return (Wtype) (a + Wtype_MIN) - Wtype_MIN;
  return (Wtype) a;
}
#endif

#ifdef L_fixunsdfsi
/* Reenable the normal types, in case limits.h needs them.  */
#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double
#undef MIN
#undef MAX
#include <limits.h>

UWtype
__fixunsdfSI (DFtype a)
{
  if (a >= - (DFtype) Wtype_MIN)
    return (Wtype) (a + Wtype_MIN) - Wtype_MIN;
  return (Wtype) a;
}
#endif

#ifdef L_fixunssfsi
/* Reenable the normal types, in case limits.h needs them.  */
#undef char
#undef short
#undef int
#undef long
#undef unsigned
#undef float
#undef double
#undef MIN
#undef MAX
#include <limits.h>

UWtype
__fixunssfSI (SFtype a)
{
  if (a >= - (SFtype) Wtype_MIN)
    return (Wtype) (a + Wtype_MIN) - Wtype_MIN;
  return (Wtype) a;
}
#endif
