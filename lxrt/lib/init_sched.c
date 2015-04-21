
/*
init_sched.c (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>

void init_linux_scheduler(int sched, int pri)
{
        struct sched_param mysched;

	if(sched != SCHED_RR && sched != SCHED_FIFO) {
                puts("Invalid scheduling scheme");
                exit(1);
	}

	if((pri < sched_get_priority_min(sched)) || (pri > sched_get_priority_max(sched))) {
		puts("Invalid priority");
		exit(2);
	}

        mysched.sched_priority = pri ;
        if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
                puts("Error in setting the Linux scheduler");
                perror("errno");
                exit(3);
        }
}
