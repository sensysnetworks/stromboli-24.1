function Code=make_standalone()
//generates some code for the standalone real time procedure
//Copyright INRIA
//Author : Rachid Djenidi
//Modified for RTAI by Paolo Mantegazza (mantegazza@aero.polimi.it)
//Modified for RTAI by Roberto Bucher (bucher@die.supsi.ch)

//Generates simulation routine for standalone simulation
  iwa=zeros(nblk,1);Z=[z;outtb;iwa]';
Code=[ ''
       cformatline('static double z[]={'+strcat(string(Z),',')+'};',70)
       '']
       ''
  Code=[Code;
	'/*'+part('-',ones(1,40))+'  Lapack messag function */ ';
	'void'
	'C2F(xerbla)(SRNAME,INFO,L)'
	'char *SRNAME;'
	'int *INFO;'
	'long int L;'
	'{'
	'#ifdef __KERNEL__'
	  'rt_printk(""** On entry to %s, parameter number %d had an illegal value\n"",SRNAME,*INFO);'
	'#else'
	  'printf(""** On entry to %s, parameter number %d had an illegal value\n"",SRNAME,*INFO);'
	'#endif'
	'}'
	''
	'#include ""'+rdnom+'_io.c""']
endfunction
