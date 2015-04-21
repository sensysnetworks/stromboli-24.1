function Code=make_sensor(standalone)
// Generating the routine for sensors interfacing
//Copyright INRIA
//Author : Rachid Djenidi
//Modified for RTAI by Roberto Bucher (bucher@die.supsi.ch)

  
Call=['/*'+part('-',ones(1,40))+' Sensor */ ';
      'void ';
      rdnom+'_sensor(flag,nport,nevprt,t,y,ny)']
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
	  '      * y     : the vector outputs value'
	  '      * ny    : the output  vector size'
	  '      */']
dcl=['     integer *flag,*nevprt,*nport;'
     '     integer *ny;'
     ''
     '     double  *t, y[];'
     '{'
     '  int k;'];
if standalone then
  a_sensor=['  switch (*flag) {'
	    '  case 1 : /* set the ouput value */'
	    '    /* sens_1_input */'
	    '    break;'
	    '  case 2 : /* Update internal discrete state if any */'
	    '    /* sens_1_update */'
	    '    break;'
	    '  case 4 : /* sensor initialisation */'
	    '    /* sens_1_init */'
	    '    break;'
	    '  case 5 : /* sensor ending */'
	    '    /* sens_1_end */'
	    '    break;'
	    '  }']
else
  a_sensor=[]
end
nc=size(cap,'*')
Code=[]
if nc==1|~standalone then
  Code=[Code;
	Call
	comments
	dcl
	a_sensor
	'}'];
elseif nc>1 then
  S='  switch (*nport) {'
  str1='sens_1'
  for k=1:nc
    str2='sens_'+string(k)
    a_sensor=strsubst(a_sensor,str1,str2)
    str1=str2
    S=[S;
       '  case '+string(k)+' : /* Port number '+string(k)+' ----------*/'
       '  '+a_sensor
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
