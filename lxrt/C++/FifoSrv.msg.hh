
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#ifndef __FIFOSRV_MSG_HH__
#define __FIFOSRV_MSG_HH__

struct Msg {
	int command;	// the command to execute
	int length;	// length of what follows
        char buffer[2048];
	};

struct Rep {
        int error;	// the reply error code
        int length;
        };

#define KPRINT_IT	0x321b 
        // error codes from the FifoSrv
#define FSRV_OK		 0
#define FSRV_ERROR	-1

#endif // __FIFOSRV_MSG_HH__
