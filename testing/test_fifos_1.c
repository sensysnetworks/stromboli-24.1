
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include <rtai_fifos.h>

#define RT_HELPER_PATH "rt_helper"

#define SIZE_CHUNK1 (PAGE_SIZE*16+100)
#define SIZE_CHUNK2 PAGE_SIZE

/* segfault catcher */

jmp_buf jump_env;

void segv_handler(int num,siginfo_t *si,void *x)
{
	longjmp(jump_env,1);
}

int test_segfault(void *memptr)
{
	volatile char tmp;
	int ret;

	ret=setjmp(jump_env);
	if(!ret)tmp = *((char *)(memptr));
	return ret;
}

int test_writefault(void *memptr)
{
	volatile char tmp;
	int ret;

	ret=test_segfault(memptr);
	if(ret)return ret;
	tmp = *((char *)(memptr));
	ret=setjmp(jump_env);
	if(!ret)*((char *)(memptr)) = tmp;
	return ret;
}


void setup_segfaulter(void)
{
	struct sigaction act;

	memset(&act,0,sizeof(act));
	act.sa_sigaction=&segv_handler;
	act.sa_flags=SA_SIGINFO;
	sigaction(SIGSEGV,&act,NULL);

}

/* end */

/* rt_helper */

int rt_helper_load(const char *module_name,char *deps[])
{
	char cmd[100];
	int i;
	int ret;

	strcpy(cmd,RT_HELPER_PATH);
	strcat(cmd," load ");
	strcat(cmd,module_name);
	for(i=0;deps[i];i++){
		strcat(cmd," ");
		strcat(cmd,deps[i]);
	}
	ret=system(cmd);
	if(ret)printf("W: rt_helper returned %d\n",ret);
	return ret;
}

int rt_helper_unload(const char *module_name)
{
	char cmd[100];
	int ret;

	strcpy(cmd,RT_HELPER_PATH);
	strcat(cmd," unload ");
	strcat(cmd,module_name);
	ret=system(cmd);
	if(ret)printf("W: rt_helper returned %d\n",ret);
	return ret;
}

int fifo_fd;

void send_cmd(int cmd)
{
	write(fifo_fd,&cmd,sizeof(cmd));
}

int main(int argc,char *argv[])
{
	int fd;
	int err;
	//int ret;
	//int tmp;
	char *depmods[]={"rtai","rtai_fifos",0};

	setup_segfaulter();

	rt_helper_load("rtai_test_fifos_1",depmods);

	printf("I: Testing openability of /dev/rtf0\n");
	fd=open("/dev/rtf0",O_RDWR);
	if(fd<0){
		err=errno;
		printf("E: Cannot open /dev/rtf0: %s\n",strerror(err));
		switch(err){
		case ENODEV:
			printf("E: rtai_fifos.o module not loaded?\n");
			break;
		}
		exit(1);
	}
	close(fd);
	printf("I: ok\n");

	rt_helper_unload("rtai_test_fifos_1");

	return 0;
}


