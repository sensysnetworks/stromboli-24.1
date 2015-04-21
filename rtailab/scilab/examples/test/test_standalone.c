#include "/usr/lib/scilab-2.7/routines/machine.h"
/* Code prototype for standalone use  */
/*     Generated by Code_Generation toolbox of Scicos with scilab-2.7 */
/*     date : 30-Aug-2003 */

void testmain1(double *z, double *t, double * rpar, integer *nrpar,
  integer *ipar,integer *nipar);

void testmain2(double *z, double *t, double * rpar, integer *nrpar,
  integer *ipar,integer *nipar) ;

void test_init(double *z, double *t, double * rpar, integer *nrpar,
  integer *ipar,integer *nipar) ;

void test_end(double *z, double *t, double * rpar, integer *nrpar,
  integer *ipar,integer * nipar) ;

void test_const_update(double *z, double *t, double * rpar,
   integer *nrpar,integer *ipar,integer * nipar) ;

void set_nevprt(int nevprt);
static double RPAR1[ ] = {
 
/* Routine name of block: dsslti
   Gui name of block: DLR_f
Exprs: 0.000009987*z+0.000009973,z^2-1.996*z+0.996
rpar= 
*/
0,-0.996,1,1.996,0,1,0.0000100,0.0000100,0,
 
/* Routine name of block: gain
   Gui name of block: GAINBLK_f
Exprs: -1.0
rpar= 
*/
-1,
 
/* Routine name of block: gain
   Gui name of block: GAINBLK_f
Exprs: 2.0
rpar= 
*/
2,
 
/* Routine name of block: cstblk
   Gui name of block: CONST_f
Exprs: -1
rpar= 
*/
-1,
 
/* Routine name of block: gain
   Gui name of block: GAINBLK_f
Exprs: 0.5
rpar= 
*/
0.5,
};
static integer NRPAR1  = 13;
static integer IPAR1[ ] = {
 
/* Routine name of block: mux
 Gui name of block: MUX_f
Compiled structure index: 4
Exprs: 2
ipar= {2};
*/
2,
};
static integer NIPAR1  = 1;

static double z[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*----------------------------------------  Lapack messag function */ 
void
C2F(xerbla)(SRNAME,INFO,L)
char *SRNAME;
int *INFO;
long int L;
{
printf("** On entry to %s, parameter number %d had an illegal value\n",SRNAME,*INFO);
}

#include "test_io.c"
