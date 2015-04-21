//-------------------------------------------------------------------------
// (C)1999 Dan Popick
// (C)1999 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#ifndef _FIFO_HH_
#define _FIFO_HH_

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <fstream>

using std::fstream;
using std::ios;

template <class fmsg, class frep>
class RTFifo {
   private:
      fstream fd;
      int fifo;
      ios::openmode flags;

   protected:
      void Open();

   public:
       RTFifo() { } // default ctor required 
      ~RTFifo() { Close(); }	

      void Init(int fifo_no, ios::openmode flag);
      void Close();                     
      
	// these methods return true if !errno 
      bool postMsg(fmsg &msg);
      bool readMsg(frep &rep);
};

template <class fmsg, class frep>
class RTBidirectionalFifo {
   private:
      RTFifo<fmsg, frep> Fifo_In, Fifo_Out;
      int base_fifo;

   public:
      void Init(int base_fifo_no) { 
           base_fifo = base_fifo_no ;
           Fifo_In.Init(base_fifo, ios::in);
           Fifo_Out.Init(base_fifo + 1, ios::out);
           }
 
      RTBidirectionalFifo(int base_fifo_no) { Init(base_fifo_no); }
      ~RTBidirectionalFifo() { }

      bool postMsg(fmsg &msg)
         { return Fifo_Out.postMsg(msg); }
      bool readMsg(frep &rep)
         { return Fifo_In.readMsg(rep); }
      bool syncMsg(fmsg &msg, frep &rep)
         { return Fifo_Out.postMsg(msg) && Fifo_In.readMsg(rep); }
};

template <class fmsg, class frep>
void
RTFifo<fmsg,frep>::Init(int fifo_no, ios::openmode flag) {
    fifo = fifo_no;
    flags = flag; 
    Open();
}

template <class fmsg, class frep>
void
RTFifo<fmsg,frep>::Open() {
   char fn[32];

   errno = 0 ; // libc6 will not set errno to 0. 
   sprintf(fn, "/dev/rtf%d", fifo);
   fd.open(fn, flags);

   if( errno ) {
        perror("Error opening rtai-fifo");
	exit(-1);
	}
}

template <class fmsg, class frep>
void
RTFifo<fmsg,frep>::Close() {
   if (fd.is_open()) fd.close();
}

template <class fmsg, class frep>
bool
RTFifo<fmsg,frep>::postMsg(fmsg &msg) {

   //	
   // Enforce length to ensure synchronisation with Server.
   //
   int *pt, len;
   pt = (int *) &msg ;
   pt++;
   len = *pt ;
   if(len < 0) return false; // should not happen 
   errno = 0 ;	
   fd.write((char *)&msg, len + 2*sizeof(int));
   if(!errno) fd.flush(); // force it into the kernel now 

   return errno ? false : true ;
}

template <class fmsg, class frep>
bool
RTFifo<fmsg,frep>::readMsg(frep &rep) {
   int *pt, len;

   pt = (int *) & rep;	
   errno = 0;
   fd.read((char *) pt, 2*sizeof(int)); // read error and length
   return  errno ? false : true ;

   pt++;
   len = *pt ;
   pt++;	

   if( len ) fd.read((char *) pt, len);
   return  errno ? false : true ;
}

#endif /* _FIFO_HH_ */
