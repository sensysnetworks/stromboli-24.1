
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

#include <rtai_shm.h>

#define RT_HELPER_PATH "rt_helper"
//#define RT_HELPER_PATH "/bin/rt_helper"


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
	int ret;
	int tmp;
	int ok;
	void *adr1,*adr2,*adr3,*adr;
	char *depmods[]={"rtai","rtai_shm","rtai_fifos",0};

	setup_segfaulter();

	rt_helper_load("rtai_test_shmem_1",depmods);

	fifo_fd=open("/dev/rtf1",O_RDWR);

	printf("I: Testing openability of /dev/rtai_shm\n");
	fd=open("/dev/rtai_shm",O_RDWR);
	if(fd<0){
		err=errno;
		printf("E: Cannot open /dev/rtai_shm: %s\n",strerror(err));
		switch(err){
		case ENODEV:
			printf("E: rtai_shm.o module not loaded?\n");
			break;
		}
		exit(1);
	}
	close(fd);
	printf("I: ok\n");

	printf("I: allocating chunk 1\n");
	adr1 = rtai_malloc(1, SIZE_CHUNK1);
	printf("rtai_malloc() returned %p\n",adr1);
	if(!adr1){
		printf("E: rtai_malloc() returned NULL!\n");
	}

	printf("I: reallocate chunk 1\n");
	adr2 = rtai_malloc(1, SIZE_CHUNK1);
	printf("rtai_malloc() returned %p\n",adr2);
	if(adr2){
		printf("E: rtai_malloc() did not return NULL\n");
	}else{
		printf("W: rtai_malloc() returned NULL, expected?\n");
	}

	printf("I: test allocated chunk for correct mapping\n");
	adr=(void *)(((unsigned long)adr1) & (~(PAGE_SIZE-1)));
	for(;adr<adr1+SIZE_CHUNK1;adr+=PAGE_SIZE){
		ret=test_segfault(adr);
		if(ret){
			printf("E: %p failed\n",adr);
		}else{
			printf("%p ok\n",adr);
		}
	}

	ok=1;
	printf("I: test allocated chunk for zero\n");
	for(adr=adr1;adr<adr1+SIZE_CHUNK1;adr++){
		tmp = *(char *)adr;
		if(tmp){
			printf("E: found %d at %p\n",tmp,adr);
			ok=0;
		}
	}
	if(ok){
		printf("I: ok\n");
	}

	printf("I: allocate chunk 2\n");
	adr3 = rtai_malloc(2, SIZE_CHUNK2);
	printf("rtai_malloc() returned %p\n",adr3);
	if(!adr3){
		printf("E: rtai_malloc() returned NULL!\n");
	}

	printf("I: free chunk 2\n");
	rtai_free(2, adr3);

	printf("I: free chunk 1\n");
	rtai_free(1, adr1);

	printf("I: testing unmap of chunk1\n");
	ret=test_segfault(adr1);
	if(ret){
		printf("ok (segfaulted)\n");
	}else{
		printf("E: area not unmapped\n");
	}

	printf("I: testing unmap of chunk2\n");
	ret=test_segfault(adr3);
	if(ret){
		printf("ok (segfaulted)\n");
	}else{
		printf("E: area not unmapped\n");
	}

	printf("I: kernel alloc\n");
	send_cmd(0);
	
	printf("I: allocating chunk 1\n");
	adr1 = rtai_malloc(1, SIZE_CHUNK1);
	printf("rtai_malloc() returned %p\n",adr1);
	if(!adr1){
		printf("E: rtai_malloc() returned NULL!\n");
	}

	printf("I: kernel touch\n");
	send_cmd(2);
	
	ret = *(int *)adr1;
	if(ret){
		printf("touched area: %d\n",ret);
	}else{
		printf("E: area untouched\n");
	}

	printf("I: free chunk 1\n");
	rtai_free(1, adr1);

	printf("I: kernel free\n");
	send_cmd(1);

	close(fifo_fd);

	//getchar();

	rt_helper_unload("rtai_test_shmem_1");

	return 0;
}


