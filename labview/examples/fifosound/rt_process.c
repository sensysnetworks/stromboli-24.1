/*
COPYRIGHT (C) 2002  Thomas Leibner (leibner@t-online.de)
                    Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#define HARDREALTIME	/* undef to debug in killable soft mode */
#define	U_LAW 		/* for a more sophistcated conversion to pc speaker, 
			   see rtai/examples/sound */
#undef	 FEED 		/* undef to let another process feed the sound data */



#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/io.h>

#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <rtai_fifos_lxrt.h>

#ifdef U_LAW
#include "pcsp_tables.h"
#endif

#define KEEP_STATIC_INLINE

#ifdef U_LAW

#define DIVISOR 2 /* 1..5, quality of sound*/ 
#define PERIOD (125000/DIVISOR)
//#define PERIOD 62500
#define STACK_SIZE 4000
static unsigned char vl_tab[256];
static int port61;

#else

#define PERIOD 125000

#endif

#define PORT_ADR 0x61



#ifdef U_LAW
static void pcsp_calc_vol(int volume)
{
	int i,j;

	for(i=0;i<256; i++){
		//vl_tab[i]=(volume*ulaw[i])>>8;
		j=i;
		if(j>128)j=382-j;
		vl_tab[i]=1+((volume*ulaw[i])>>8);
	}
}
#else
static int filter(int x)
{
	static int oldx;
	int ret;

	if (x & 0x80) {
		x = 382 - x;
	}
	ret = x > oldx;
	oldx = x;
	return ret;
}
#endif

static void *intr_handler(void *args)
{
	RT_TASK *mytask;
	RTIME period;
	int playfifo, cntrfifo;
	char data;
#ifdef U_LAW
	int go=1;
	int divisor = DIVISOR;
#else
	char temp;
#endif
	rt_allow_nonroot_hrt();
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

#ifdef U_LAW
	if (iopl(3)) {
	  printf("iopl() failed/n");
	  exit(1);
	}  
	outb_p(0x92, 0x43); /* binary, mode1, LSB only, ch 2 */
	/* You can make this bigger, but then you start to get
	 * clipping, which sounds bad.  29 is good */

	/* VOLUME SETTING */
	pcsp_calc_vol(70);

	port61 = inb(0x61) | 0x3;
#else
	ioperm(PORT_ADR, 1, 1);
#endif

 	if (!(mytask = rt_task_init_schmod(nam2num("SOUND"), 1, 0, 0, SCHED_FIFO, 0xF))) {
		printf("CANNOT INIT SOUND TASK\n");
		exit(1);
	}
	playfifo = 0;
	cntrfifo = 1;
	rtf_create(playfifo, 8192); /* 1s buffer */
	rtf_reset(playfifo);
	rtf_create(cntrfifo, 1000);
	rtf_reset(cntrfifo);

	rt_set_oneshot_mode();
	start_rt_timer(0);
	period = nano2count(PERIOD);

	printf("\nINIT SOUND TASK\n");

	mlockall(MCL_CURRENT | MCL_FUTURE);
	
#ifdef HARDREALTIME
	rt_make_hard_real_time();
#endif	
	rt_task_make_periodic(mytask, rt_get_time() + 5*period, period);
	rtf_put(cntrfifo, &data, 1);

	rt_sleep(nano2count(100000000));

	while(1) {
#ifdef U_LAW
	  if (!(--divisor)) {
	    divisor = DIVISOR;
	    //cpu_used[hard_cpu_id()]++;
	    if (!(rtf_get(playfifo, &data, 1) > 0)) {
	      go=0;
	    }else{
	      go=1;
	    }
	  }
	  if(go){
	    outb(port61,0x61);
	    outb(port61^1,0x61);
	    outb(vl_tab[((unsigned int)data)&0xff], 0x42);
	  }
#else
	  if (rtf_get(playfifo, &data, 1) > 0) {
	    go=1;
	    data = filter(data);
	    temp = inb(PORT_ADR);            
	    temp &= 0xfd;
	    temp |= (data & 1) << 1;
	    outb(temp, PORT_ADR);
	  } else {
	    go=0;
	  }	
#endif
	  rt_task_wait_period();
	  if (go==0) { 
	  	if (rtf_get(cntrfifo, &data, 1) > 0) {
	    		break;
	    	}		
	  } 
	}
	
	stop_rt_timer();
	rt_make_soft_real_time();
	rtf_destroy(playfifo);
	rtf_destroy(cntrfifo);
	rt_task_delete(mytask);
	printf("\nEND SOUND TASK\n");
	return 0;
}


static pthread_t thread;
static int end;

static void endme(int dummy) { end = 1; }

int main(void)
	{
#ifdef FEED
	unsigned int player;
	int playfifo, cntrfifo;
	char data;
#endif	
 
	signal(SIGINT, endme);
	
#ifdef FEED
	if ((player = open("linux.au", O_RDONLY)) < 0) {
		printf("ERROR OPENING SOUND FILE (linux.au)\n");
		exit(1);
	}
	if ((playfifo = rtf_open_sized("/dev/rtf0", O_RDWR, 2000)) < 0) {
		printf("ERROR OPENING FIFO0\n");
		exit(1);
	}
	if ((cntrfifo = open("/dev/rtf1", O_RDWR)) < 0) {
		printf("ERROR OPENING FIFO1\n");
		exit(1);
	}

	pthread_create(&thread, NULL, intr_handler, NULL);

	read(cntrfifo, &data, 1);
	printf("\nINIT MASTER TASK\n\n(CTRL-C TO END EVERYTHING)\n");

	while (!end) {	
		lseek(player, 0, SEEK_SET);
		while(!end && read(player, &data, 1) > 0) {
			write(playfifo, &data, 1);
		}
	}

	write(cntrfifo, &data, 1); 
	close(playfifo);
	close(cntrfifo);
	close(player);
	printf("\nEND MASTER TASK\n");
        pthread_join(thread, NULL);
#else
	(*intr_handler)(NULL);
#endif

	return 0;
}
