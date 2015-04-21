function ok=gen_ccode();
//Generates the C code for new block simulation
  
//Copyright INRIA
//Author : Rachid Djenidi
//Modified for RTAI by Paolo Mantegazza (mantegazza@aero.polimi.it)

  [CCode,FCode]=gen_blocks()
  Code=[make_decl();
	Protos;
	make_static()
	make_computational() 
	make_main1();
	make_main2();
	make_init();
	make_end();
	make_const_update()
	c_make_doit1();
	c_make_doit2();
	c_make_outtb();
	c_make_initi();
	c_make_endi()]
  ierr=execstr('mputl(Code,rpat+''/''+rdnom+''.c'')','errcatch')
  if ierr<>0 then
    message(lasterror())
    ok=%f
    return
  end

  if FCode<>[] then
    ierr=execstr('mputl(FCode,rpat+''/''+rdnom+''f.f'')','errcatch')
    if ierr<>0 then
      message(lasterror())
      ok=%f
      return
    end
  end

  Code=['#include '"'+SCI+'/routines/machine.h'"';
	make_actuator(%t);
	make_sensor(%t);
	make_upar_update()]
  ierr=execstr('mputl(Code,rpat+''/''+rdnom+''_void_io.c'')','errcatch')
  if ierr<>0 then
    message(lasterror())
    ok=%f
    return
  end
 
  Code=['#include '"'+SCI+'/routines/machine.h'"';
	make_decl_standalone()
	make_static_standalone()
        make_standalone()]
  ierr=execstr('mputl(Code,rpat+''/''+rdnom+''_standalone.c'')','errcatch')
  if ierr<>0 then
    message(lasterror())
    ok=%f
    return
  end
     
endfunction
