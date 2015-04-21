
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#include "hrt.hh"

extern "C" {
//    #include <malloc.h>

    extern void dump_malloc_stats(void);
}


void Recursive(int index)
{
char *pt, buf[1024];

index++;
pt     = new char[150000];
buf[0] = 0;

rt_sleep(nano2count( 10000 ));
rtai_print_to_screen(".");
if (index < 128) Recursive(index);

delete pt;
}

int main( int argc, char *argv[])
{
Hrt *me;

rt_set_oneshot_mode();
start_rt_timer(0);
 
// We enter hard real time mode with a LARGE stack and a *BIG* heap!
me = new Hrt( 136*1024, 20*1024*1024, "TSTMEM" );

rtai_print_to_screen("Cookie monster eats all the memory...\n");
Recursive(0);
dump_malloc_stats();

delete me;
stop_rt_timer();
}
