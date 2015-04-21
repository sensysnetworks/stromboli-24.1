function [x,y,typ]=test_c(job,arg1,arg2)
// Copyright INRIA
x=[];y=[];typ=[];
select job
case 'plot' then
  standard_draw(arg1)
case 'getinputs' then
  [x,y,typ]=standard_inputs(arg1)
case 'getoutputs' then
  [x,y,typ]=standard_outputs(arg1)
case 'getorigin' then
  [x,y]=standard_origin(arg1)
case 'set' then
  x=arg1;
  graphics=arg1.graphics;label=graphics.exprs
  model=arg1.model;
case 'define' then
  in = 1
  out = [2;1;1;1]
  z = [0;0]
  outtb = [0;0;0;0;0;0;0;0;0;0]
  iwa=zeros(2,1)
  Z=[z;outtb;iwa]
  rpar = [0;-0.996;1;1.996;0;1;0.0000100;0.0000100;0;-1;2;-1;0.5]
  ipar = [2;1;1;1;2;3;4]
  clkinput = 1
  model=scicos_model(sim=list('test',1),in=in,out=out,..
          evtin=clkinput,dstate=Z,rpar=rpar,ipar=ipar,..
          blocktype='c',dep_ut=[%t %f])
  label=string(in)
  gr_i='xstringb(orig(1),orig(2),''test'',sz(1),..
         sz(2),''fill'')'
  x=standard_define([2 2],model,label,gr_i)
end
endfunction
