
//-------------------------------------------------------------------------
// (C)2000 Pierre Cloutier
// (C)2000 Poseidon Controls Inc.
//
// pcloutier@PoseidonControls.com                                    LGPLv2
//-------------------------------------------------------------------------

#include "hrt.hh"

extern "C" {
    #include <malloc.h>

    extern void dump_malloc_stats(void);
}

Hrt::Hrt(int sk, int hp, char *name)
{
	lock_all(stack=sk, heap=hp);

//	dump_malloc_stats();

	init_linux_scheduler( SCHED_FIFO, 99);

	rt_set_oneshot_mode();
	start_rt_timer(0);

	task = rt_task_init((int) nam2num(name), 0, 0, 0);
	rt_make_hard_real_time();
	rtai_print_to_screen("\nHRT mode starts\n");
}
