function [x,y,typ]=Controller_c(job,arg1,arg2)
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
  graphics=arg1(2);label=graphics(4)
  model=arg1(3);
case 'define' then
  in = [1;1]
  out = [1;1]
  z = [0;0]
  outtb = [0;0;0;0;0;0;0;0]
  iwa=zeros(11,1)
  Z=[z;outtb;iwa]
  rpar = [-0.3;1;1.3;0;-0.3;1;1.3;0;0.5;1]
  ipar = [1;1;1;2;2]
  clkinput = 1
  model=list(list('Controller',1),in,out,clkinput,[],[],Z,rpar,ipar,..
      'c',[],[%t %f],' ',list())
  label=string(in)
  gr_i='xstringb(orig(1),orig(2),''Controller'',sz(1),sz(2),''fill'')'
  x=standard_define([2 2],model,label,gr_i)
end
endfunction
