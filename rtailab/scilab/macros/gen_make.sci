function Makename=gen_make(name,files,libs,Makename)
//Modified for RTAI by Roberto Bucher (bucher@die.supsi.ch)

  if getenv('WIN32','NO')=='OK' then
    Makename=gen_make_win32(name,files,libs)
  else
    Makename=gen_make_unix(name,files,libs)
  end
endfunction

function Makename=gen_make_unix(name,files,libs,Makename)
  T=["# generated by builder.sce: Please do not edit this file"
     "# ------------------------------------------------------"
     "krt: rt_process.o"
     ""
     "TARGET  = rt_process.o"
     "RTAIDIR = /home/rtai4"
     "RTSCIDIR = /usr/local/scicos"
     "SCIDIR = "+SCI
     ""
     "MODEL = "+name
     "OBJS = "+strcat(files+'.o',' ')
     "OBJSSTAN = "+strcat(strsubst(files,name+'_void_io','urtmain')+'.o',' ')
     "KOBJSSTAN=$(MODEL).o krtmain.o"
     ""
     "SCILIBS = $(SCIDIR)/libs/scicos.a $(SCIDIR)/libs/lapack.a "+..
                "$(SCIDIR)/libs/poly.a $(SCIDIR)/libs/calelm.a "+..
                "$(SCIDIR)/libs/blas.a $(SCIDIR)/libs/lapack.a"
     "LIBRARY = lib$(MODEL)"
     "OTHERLIBS = "+libs
     "KLIBRARY = $(RTSCIDIR)/klibsci.a"
     "ULIBRARY = $(RTSCIDIR)/ulibsci.a"
     ""
     "include $(SCIDIR)/Makefile.incl";
     ""
     "CFLAGS = $(CC_OPTIONS) -O2 -I$(SCIDIR)/routines/ -I$(RTAIDIR)/include/ -I$(RTSCIDIR)/include -DMODEL=$(MODEL)"    
     "FFLAGS = $(FC_OPTIONS) -O2 -I$(SCIDIR)/routines/"
     "include $(SCIDIR)/config/Makeso.incl"
     ""
     "LINUX_HOME = /usr/src/linux"
     "INC_LNX    = -I$(LINUX_HOME)/include"
     "INC_RTA    = -I$(RTAIDIR)/include"
     "INC_SCI    = -I$(RTSCIDIR)/include"
     "INC_RTAI   = $(INC_RTA) $(INC_LNX) $(INC_SCI)"
     "C_FLAGS = -I. $(INC_RTAI) -O2 -ffast-math -D__KERNEL__ -DMODULE -DMODEL=$(MODEL)"
     ""
     "urtmain.c: $(RTSCIDIR)/urtmain.c $(MODEL).c $(MODEL)_io.c"
     ascii(9)+"cp $< ."
     ""
     "krtmain.o: $(RTSCIDIR)/krtmain.c $(MODEL).c $(MODEL)_io.c"
     ascii(9)+"cc -c $(C_FLAGS) -o $@ $<"
     ""
     "$(TARGET): $(KOBJSSTAN)"
      ascii(9)+"ld -r  -o $@  $(KOBJSSTAN) $(SCILIBS) $(KLIBRARY)"
     ""
     name+"_standalone: $(OBJSSTAN)"
      ascii(9)+"f77 $(FFLAGS) -o $@  $(OBJSSTAN) $(ULIBRARY) $(SCILIBS) -lpthread"];
  mputl(T,Makename)
endfunction


function Makename=gen_make_win32(name,files,libs,Makename)
WSCI=strsubst(SCI,'/','\') 
  T=["# generated by builder.sce: Please do not edit this file"
     "# ------------------------------------------------------"
     "SHELL = /bin/sh"
     "SCIDIR = "+SCI
     "SCIDIR1 = "+WSCI
     "SCILIBS = """+WSCI+"\bin\LibScilab.lib"""
     "LIBRARY = lib"+name
     "OBJS = "+strcat(files+'.obj',' ')
     "OBJSSTAN="+strcat(strsubst(files,'_void_io','_standalone')+'.obj',' ')
     "OTHERLIBS = "+libs
     ""
     "DUMPEXTS="""+WSCI+"\bin\dumpexts"""
     "SCIIMPLIB="""+WSCI+"\bin\LibScilab.lib"""
     ""
     "all::"
     "CC=cl"
     "LINKER=link"
     "LINKER_FLAGS=/NOLOGO /machine:ix86"
     "INCLUDES=-I"""+WSCI+"\routines\f2c""" 
     "CC_COMMON=-D__MSC__ -DWIN32 -c -DSTRICT -nologo $(INCLUDES)" 
     "CC_OPTIONS = $(CC_COMMON) -Od  -GB -Gd -W3"
     "CC_LDFLAGS = "
     "CFLAGS = $(CC_OPTIONS) -DFORDLL -I"""+WSCI+"\routines"""
     "FFLAGS = $(FC_OPTIONS) -DFORDLL -I"""+WSCI+"\routines"""
     ""
     "all :: $(LIBRARY).dll"
     " "
     "$(LIBRARY).dll: $(OBJS)"
     ascii(9)+"@echo Creation of dll $(LIBRARY).dll and import lib from ..."
     ascii(9)+"@echo $(OBJS)"
     ascii(9)+"@$(DUMPEXTS) -o ""$*.def"" ""$*.dll"" $**"
     ascii(9)+"@$(LINKER) $(LINKER_FLAGS) $(OBJS) $(SCIIMPLIB) $(XLIBSBIN) $(TERMCAPLIB) /nologo /dll /out:""$*.dll"" /implib:""$*.ilib"" /def:""$*.def""" 
     ".c.obj:"
     ascii(9)+"@echo ------------- Compile file $< --------------"
     ascii(9)+"$(CC) $(CFLAGS) $< "
     ".f.obj:"
     ascii(9)+"@echo ----------- Compile file $*.f (using f2c) -------------"
     ascii(9)+"@"""+WSCI+"\bin\f2c.exe"" $(FFLAGS) $*.f "
     ascii(9)+"@$(CC) $(CFLAGS) $*.c "
     ascii(9)+"@del $*.c "
     "clean::"
     ascii(9)+"@del *.CKP "
     ascii(9)+"@del *.ln "
     ascii(9)+"@del *.BAK "
     ascii(9)+"@del *.bak "
     ascii(9)+"@del *.def"
     ascii(9)+"@del *.dll"
     ascii(9)+"@del *.exp"
     ascii(9)+"@del *.ilib"
     ascii(9)+"@del errs" 
     ascii(9)+"@del *~ "
     ascii(9)+"@del *.obj"
     ascii(9)+"@del .emacs_* "
     ascii(9)+"@del tags "
     ascii(9)+"@del tags "
     ascii(9)+"@del TAGS "
     ascii(9)+"@del make.log "
     ""
     "distclean:: clean "
     " "
     "standalone: $(OBJSSTAN) $(OTHERLIBS) "
      ascii(9)+"$(LINKER) $(LINKER_FLAGS)  $(OBJSSTAN) $(OTHERLIBS) $(SCILIBS)  /out:standalone.exe "]

  select getenv('COMPILER','NO');
    case 'VC++'   then 
    makename = strsubst(Makename,'/','\')+'.mak'
    case 'ABSOFT' then 
    makename = strsubst(Makename,'/','\')+'.amk'
  end
  mputl(T,makename)   
endfunction
