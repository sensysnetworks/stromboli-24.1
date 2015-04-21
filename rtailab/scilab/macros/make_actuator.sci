function Code=make_actuator(standalone)
// Generating the routine for actuators interfacing
//Copyright INRIA
//Author : Rachid Djenidi
//Modified for RTAI by Roberto Bucher (bucher@die.supsi.ch)
  
Call=['/*'+part('-',ones(1,40))+' Actuators */ ';
      'void ';
      rdnom+'_actuator(flag,nport,nevprt,t,u,nu)']
comments=['     /*'
	  '      * To be customized for standalone execution';
	  '      * flag  : specifies the action to be done'
	  '      * nport : specifies the  index of the Super Bloc '
	  '      *         regular input (The input ports are numbered '
	  '      *         from the top to the bottom ) '
	  '      * nevprt: indicates if an activation had been received'
	  '      *         0 = no activation'
	  '      *         1 = activation'
	  '      * t     : the current time value'
	  '      * u     : the vector inputs value'
	  '      * nu    : the input  vector size'
	  '      */']

dcl=['     integer *flag,*nevprt,*nport;'
     '     integer *nu;'
     ''
     '     double  *t, u[];'
     '{'
     '  int k;'];

if standalone then
  a_actuator=['  /* skeleton to be customized */'
	      '  switch (*flag) {'
	      '  case 2 : '
	      '    if(*nevprt>0) {/* get the input value */'
	      '	      /* att_1_output */'
	      '    } '
	      '    break;'
	      '  case 4 : /* actuator initialisation */'
	      '    /* att_1_init */'
	      '    break;'
	      '  case 5 : /* actuator ending */'
	      '    /* att_1_end */'
	      '    break;'
	      '  }']
else
 a_actuator=[]
end
nc=size(act,'*')
Code=[]
if nc==1|~standalone then
  Code=[Call
	comments
	dcl
	a_actuator
	'}']
elseif nc>1 then
  S='  switch (*nport) {'
  str1='att_1'
  for k=1:nc
    str2='att_'+string(k)
    a_actuator=strsubst(a_actuator,str1,str2)
    str1=str2
    S=[S;
       '  case '+string(k)+' :/* Port number '+string(k)+' ----------*/'
       '  '+a_actuator
       '  break;']
  end
  S=[S;'  }']  
  Code=[Code
	Call
	comments
	dcl
	S
	'}']
end
endfunction

