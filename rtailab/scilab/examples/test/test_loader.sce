//exec file used to load the "compiled" block into Scilab
rdnom='test'
// get the absolute path of this loader file
DIR=get_absolute_file_path(rdnom+'_loader.sce')
Makename = DIR+rdnom+'_Makefile';
select COMPILER
case 'VC++'   then 
  Makename = strsubst(Makename,'/','\')+'.mak';
case 'ABSOFT' then 
  Makename = strsubst(Makename,'/','\')+'.amk';
end
//unlink if necessary
[a,b]=c_link(rdnom); while a ;ulink(b);[a,b]=c_link(rdnom);end
libn=ilib_compile('libtest',Makename)
if MSDOS then
  fileso=strsubst(libn,'/','\')
else
  fileso=strsubst(libn,'\','/')
end
link(fileso,rdnom,'c')
//load the gui function
getf(DIR+'/'+rdnom+'_c.sci');
