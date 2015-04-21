
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#include <iostream>
#include <string.h>
#include "Fifo.hh"
#include "FifoSrv.msg.hh"

using std::cin;
                                                                               
int main(int argc,char *argv[])
{
Msg m; Rep r;
RTBidirectionalFifo<Msg,Rep> *ipc = new RTBidirectionalFifo<Msg,Rep>(1);

	while(1) {
		cin.getline((char *)m.buffer, 256, '\n');
		if(cin.eof()) break;
		m.command = KPRINT_IT ; 
		m.length  = strlen(m.buffer) + 1 ;
		ipc->syncMsg(m,r);
	}

	return 0;
}
