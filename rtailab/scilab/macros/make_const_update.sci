function Code=make_const_update()
//updates constants after a change on the fly
//Copyright INRIA
//Author : Rachid Djenidi
//Modified for RTAI by Paolo Mantegazza (mantegazza@aero.polimi.it)

  nordcl=size(ordclk,1);
  Code=['/*'+part('-',ones(1,40))+' Scicos real constants update */ ';
	'void '
	cformatline(rdnom+'_const_update(double *z, double *t, double * rpar, '+..
		    'integer *nrpar,integer *ipar,integer *nipar)',70);
	'{'];
  if size(pointi,1) <> 0 then
    Code($+1)='  integer pointi[ ]={'+strcat(string(pointi),",")+'};'; 
  else
    Code($+1)='  integer pointi[1];';
  end

  Code=[Code;
	'integer nordcl = '+string(nordcl)+';';
	'  /*Block initializations*/'
	'  /*Constants propagation*/'
	cformatline('  '+rdnom+'_outtb(z, zptr, t, tevts, evtspt, nevts, '+..
		    'pointi, outptr, clkptr, ordptr, ordclk, &nordcl, rpar, ipar, '+..
		    'rdfunptr,funtyp, &(z['+string(size(z,1))+']), '+..
		    '(int *)(z+'+string(size(z,1)+size(outtb,1))+'));',70);
       '} '];
  
endfunction
