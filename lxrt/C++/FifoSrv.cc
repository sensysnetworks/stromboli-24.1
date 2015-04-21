
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#include "FifoSrv.hh"

static FifoSrv *cppThis[8];

int the_handler(unsigned int fifo)
{
	FifoSrv *Srv;
        int len, err;

	Srv = cppThis[fifo];
	Srv->CmdHdr.length = Srv->CmdHdr.command = 0;
        while((err=rtf_get(Srv->RXF, &Srv->CmdHdr, 2*sizeof(int)))==2*sizeof(int)) {
                if( Srv->CmdHdr.length ) {
                        // get more data if need be 
                        len = rtf_get(Srv->RXF, Srv->CmdHdr.buffer, Srv->CmdHdr.length);
                        if(len != Srv->CmdHdr.length ) {
                                // Ooops, we're out of sync
				rt_printk("FifoSrv out of sync: len %d Srv->CmdHdr.length %d\n", len, Srv->CmdHdr.length);
				rtf_reset(Srv->RXF);
                                rtf_reset(Srv->TXF);
				continue;	        
	                        }
		}
	Srv->resume();
	}
	return 0;
}

FifoSrv::FifoSrv(char *MyName, int fifo_base, int stk, int pri, RTIME per)
      :Task(stk, pri, per) {

	TXF = fifo_base;
	RXF = TXF + 1 ;

	cppThis[RXF] = this ;
	cppThis[TXF] = this ;

	rtf_create(RXF, 2048);
	rtf_create(TXF, 2048);
	rtf_reset(RXF);
	rtf_reset(TXF);
}

FifoSrv::~FifoSrv() {
	suspend(); // Huh? cleanup_module() does this.	
	user_cleanup();	
}

void
FifoSrv::processMessage() {

//	rt_printk( "FifoSrv received cmd %d len %d\n", CmdHdr.command, CmdHdr.length);
	RepHdr.length =       0;
	RepHdr.error  = FSRV_OK;

	switch( CmdHdr.command ) {
		case KPRINT_IT:
			CmdHdr.buffer[CmdHdr.length-1]=0;
			rt_printk("%s\n", CmdHdr.buffer);
			break;

		default:
		break;
	}
}

void 
FifoSrv::user_init() {
	rtf_create_handler(RXF, the_handler);
}

void 
FifoSrv::user_execute() {
	for(;!terminated;) {	
		suspend();
		processMessage();
		rtf_put(TXF, &RepHdr, sizeof(RepHdr) + RepHdr.length);
	}
}

void
FifoSrv::user_cleanup() {
	rtf_destroy(RXF);
	rtf_destroy(TXF);
	cppThis[RXF] = ((FifoSrv *)0);
	cppThis[TXF] = ((FifoSrv *)0);
}

