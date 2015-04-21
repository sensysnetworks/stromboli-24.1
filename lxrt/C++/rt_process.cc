
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#include "FifoSrv.hh"

static Task *Srv;

extern "C" int
init_module(void) {
 	
	Srv = new FifoSrv( "Srv.0", 1, 8192, 10, 0 );
	Srv->start();

	return 0;
}

extern "C" void
cleanup_module(void) {
	delete Srv;
}
