/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include "clock.h"

#define TICK_PERIOD     100000LL    /*  0.1 msec (  1  tick) */ 
#define POLLING_DELAY  1000000LL    /*    1 msec ( 10 ticks) */
#define ONE_UNIT      10000000LL    /*   10 msec (100 ticks) */
#define FIVE_SECONDS  5000000000LL

static MBX *Keyboard;
static MBX *Screen;

static BOOLEAN hide;
static BOOLEAN Pause;

static void *ClockChrono_Read(void *args)
{
	RT_TASK *mytask;
	char ch;
	unsigned int run = 0;

	struct sched_param mysched;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
		exit(1);
 	}       
	mlockall(MCL_CURRENT | MCL_FUTURE);

 	if (!(mytask = rt_task_init(nam2num("READ"), 1, 0, 0))) {
		printf("CANNOT INIT TASK ClockChronoRead\n");
		exit(1);
	}
	printf("INIT TASK ClockChronoRead %p.\n", mytask);

	while(1) {
		rt_mbx_receive(Keyboard, &ch, 1);
		ch = toupper(ch);
		switch(ch) {
			case 'T': case 'R': case 'H': case 'M': case 'S':
				CommandClock_Put(ch);
				break;
			case 'C': case 'I': case 'E':
				CommandChrono_Put(ch);
				break;
			case 'N':
				hide = ~hide;
				break;
			case 'P':
				Pause = TRUE;
				rt_sleep(nano2count(FIVE_SECONDS));
				Pause = FALSE;
				break;
			case 'K': case 'D':
				run += ch;
				if (run == ('K' + 'D')) {
					rt_send(rt_get_adr(nam2num("CLOCK")), run);
					rt_send(rt_get_adr(nam2num("CHRONO")), run);
				}
				break;
			case 'F':
				CommandChrono_Put('F');
				CommandClock_Put('F');
				goto end;
		}
	}
end:
	rt_task_delete(mytask);
	printf("END TASK ClockChronoRead %p.\n", mytask);
	return 0;
}

static void *ClockChrono_Clock(void *args)
{
	RT_TASK *mytask;
	RTIME OneUnit = nano2count(ONE_UNIT);
	int hundredthes = FALSE;
	MenageHmsh_tHour hour;
	MenageHmsh_tChain11 hourChain;
	char command;
	BOOLEAN display;
	unsigned int msg;

	struct sched_param mysched;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
		exit(1);
 	}       
	mlockall(MCL_CURRENT | MCL_FUTURE);

 	if (!(mytask = rt_task_init_schmod(nam2num("CLOCK"), 1, 0, 0, SCHED_FIFO, 0x1))) {
		printf("CANNOT INIT TASK ClockChronoClock\n");
		exit(1);
	}
	printf("INIT TASK ClockChronoClock %p.\n", mytask);

	rt_make_hard_real_time();
	rt_receive(rt_get_adr(nam2num("READ")), &msg);
	MenageHmsh_Initialise(&hour);
	MenageHmsh_Convert(hour, hundredthes, &hourChain);
	Display_PutHour(hourChain);
	while(1) {
		CommandClock_Get(&command);
		switch(command) {
			case 'R':
				rt_sleep(OneUnit);
				MenageHmsh_PlusOneUnit(&hour, &display);
				break;
			case 'T': 
				MenageHmsh_InitialiseHundredthes(&hour);
				display = FALSE;
				break;
			case 'H': 
				MenageHmsh_AdvanceHours(&hour);
				display = TRUE;
				break;
			case 'M':
				MenageHmsh_AdvanceMinutes(&hour);
				display = TRUE;
				break;
			case 'S':
				MenageHmsh_AdvanceSeconds(&hour);
				display = TRUE;
				break;
			case 'F':
				goto end;
			}
		if (display) {
			MenageHmsh_Convert(hour, hundredthes, &hourChain);
			Display_PutHour(hourChain);
		}
	}
end:
	rt_make_soft_real_time();
	hourChain.chain[1] = 101;
	Display_PutHour(hourChain);
 	rt_task_delete(mytask);
	printf("END TASK ClockChronoClock %p.\n", mytask);
	return 0;
}

static void *ClockChrono_Chrono(void *args)
{
	RT_TASK *mytask;
	RTIME OneUnit = nano2count(ONE_UNIT);
	MenageHmsh_tHour times;			
	MenageHmsh_tChain11 timesChain;
	BOOLEAN Intermediatetimes = FALSE;
	MenageHmsh_tHour endIntermediateTimes;
	BOOLEAN display;
	BOOLEAN hundredthes = FALSE;
	char command;
	unsigned int msg;

	struct sched_param mysched;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
		exit(1);
 	}       
	mlockall(MCL_CURRENT | MCL_FUTURE);

 	if (!(mytask = rt_task_init_schmod(nam2num("CHRONO"), 1, 0, 0, SCHED_FIFO, 0x2))) {
		printf("CANNOT INIT TASK ClockChronoChrono\n");
		exit(1);
	}
	printf("INIT TASK ClockChronoChrono %p.\n", mytask);

	rt_make_hard_real_time();
	rt_receive(rt_get_adr(nam2num("READ")), &msg);
	command = 'R';
	while(1) {
		switch(command) {
			case 'R':
				MenageHmsh_Initialise(&times);
				display = TRUE;
				hundredthes = FALSE;
				Intermediatetimes = FALSE;
				break;
			case 'C':
				rt_sleep(OneUnit);
				MenageHmsh_PlusOneUnit(&times, &display);
				if (Intermediatetimes) {
					Intermediatetimes = !MenageHmsh_Equal(
						times, endIntermediateTimes);
					display = !Intermediatetimes;
					hundredthes = FALSE;
				}
				break;
			case 'I':
				Intermediatetimes = TRUE;
				endIntermediateTimes = times;
				MenageHmsh_PlusNSeconds(3, 
							&endIntermediateTimes);
				display = TRUE;
				hundredthes = TRUE;
				break;
			case 'E':
				display = TRUE;
				hundredthes = TRUE;
				break;
			case 'F':
				goto end;
		}
		if (display) {
			MenageHmsh_Convert(times, hundredthes, &timesChain);
			Display_PutTimes(timesChain);
		}
		CommandChrono_Get(&command);
	}
end:
	rt_make_soft_real_time();
 	rt_task_delete(mytask);
	printf("END TASK ClockChronoChrono %p.\n", mytask);
	return 0;
}

static void *ClockChrono_Write(void *args)
{
	RT_TASK *mytask;
	Display_tDest receiver;
	MenageHmsh_tChain11 chain;
	//char buf[25] = "00:00:00   00:00:00:00";

	struct sched_param mysched;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
		exit(1);
 	}       
	mlockall(MCL_CURRENT | MCL_FUTURE);

 	if (!(mytask = rt_task_init(nam2num("WRITE"), 1, 0, 0))) {
		printf("CANNOT INIT TASK ClockChronoWrite\n");
		exit(1);
	}
	printf("INIT TASK ClockChronoWrite %p.\n", mytask);

	while(1) {
		Display_Get(&chain, &receiver);
		if (chain.chain[1] == 101) {
			goto end;
		}
		if (!hide && !Pause) {
			rt_mbx_send_if(Screen, chain.chain, 12);
		}
	}
end:
 	rt_task_delete(mytask);
	printf("END TASK ClockChronoWrite %p.\n", mytask);
	return 0;
}

static pthread_t Read;
static pthread_t Clock;
static pthread_t Chrono;
static pthread_t Write;
static pthread_t Clock_task;
static pthread_t Chrono_task;
static pthread_t Disp_task;

extern void *CommandClock_task(void *args);
extern void *CommandChrono_task(void *args);
extern void *Display_task(void *args);

int main(void)
{
	RT_TASK *mytask;
	hide = FALSE;
	Pause = FALSE;
	Keyboard = rt_mbx_init(nam2num("KEYBRD"), 1000);
	Screen = rt_mbx_init(nam2num("SCREEN"), 1000);

 	if (!(mytask = rt_task_init(nam2num("MASTER"), 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}
	printf("\nINIT MASTER TASK %p.\n", mytask);

	rt_set_oneshot_mode();
	start_rt_timer(0);

	if (pthread_create(&Disp_task, NULL, Display_task, NULL)) { 
		printf("ERROR IN CREATING Display_task\n");
		exit(1);
 	}       

	if (pthread_create(&Chrono_task, NULL, CommandChrono_task, NULL)) { 
		printf("ERROR IN CREATING CommandChrono_task\n");
		exit(1);
 	}       

	if (pthread_create(&Clock_task, NULL, CommandClock_task, NULL)) { 
		printf("ERROR IN CREATING CommandClock_task\n");
		exit(1);
 	}       

	if (pthread_create(&Read, NULL, ClockChrono_Read, NULL)) { 
		printf("ERROR IN CREATING ClockChrono_Read\n");
		exit(1);
 	}       

	if (pthread_create(&Chrono, NULL, ClockChrono_Chrono, NULL)) { 
		printf("ERROR IN CREATING ClockChrono_Chrono\n");
		exit(1);
 	}       

	if (pthread_create(&Clock, NULL, ClockChrono_Clock, NULL)) { 
		printf("ERROR IN CREATING ClockChrono_Clock\n");
		exit(1);
 	}       


	if (pthread_create(&Write, NULL, ClockChrono_Write, NULL)) { 
		printf("ERROR IN CREATING ClockChrono_Write\n");
		exit(1);
 	}       

	rt_task_suspend(mytask);

	while (rt_get_adr(nam2num("READ"))) {
		rt_sleep(nano2count(1000000));
	}
	while (rt_get_adr(nam2num("CLOCK"))) {
		rt_sleep(nano2count(1000000));
	}
	while (rt_get_adr(nam2num("CHRONO"))) {
		rt_sleep(nano2count(1000000));
	}
	while (rt_get_adr(nam2num("WRITE"))) {
		rt_sleep(nano2count(1000000));
	}
	while (rt_get_adr(nam2num("CLKTSK"))) {
		rt_sleep(nano2count(1000000));
	}
	while (rt_get_adr(nam2num("CHRTSK"))) {
		rt_sleep(nano2count(1000000));
	}
	while (rt_get_adr(nam2num("DSPTSK"))) {
		rt_sleep(nano2count(1000000));
	}

	rt_mbx_delete(Keyboard);
	rt_mbx_delete(Screen);
	rt_task_delete(mytask);
	printf("\nEND MASTER TASK %p.\n", mytask);
	pthread_join(Disp_task, NULL);
	pthread_join(Chrono_task, NULL);
	pthread_join(Clock_task, NULL);
	pthread_join(Chrono, NULL);
	pthread_join(Clock, NULL);
	pthread_join(Read, NULL);
	pthread_join(Write, NULL);
	return 0;
}
