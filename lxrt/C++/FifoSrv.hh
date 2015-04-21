
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//
// 29-10-2001 Added missing function prototypes - ER 
//
//-------------------------------------------------------------------------

#ifndef __FIFO_SRV_HH__
#define __FIFO_SRV_HH__

extern "C" {

  #include <sys/types.h>
  #include <rtai_fifos.h>
}

#include "task.hh"
#include "FifoSrv.msg.hh"

int the_handler(unsigned int fifo);

class FifoSrv: public Task {
   private:
        int RXF, TXF;
	Msg CmdHdr;
	Rep RepHdr;
	
   protected:
	void processMessage();
	void user_init();
	void user_execute();
	void user_cleanup();
  
   public:
	virtual ~FifoSrv();
	FifoSrv(char *MyName, int fifo_base, int stk, int pri, RTIME time);
	friend int the_handler(unsigned int fifo);
};

#endif // __FIFO_SRV_HH__ 
