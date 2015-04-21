
/*
rtai_lxrt.c (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
*/

#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> // libc5 does not need this file. libc6 does. 

/* Added for the implementation of gettimeofday_rt */
unsigned long long orig_tsc_value;

#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <rtai_fifos_lxrt.h>

/* Huh, where's the code? */
