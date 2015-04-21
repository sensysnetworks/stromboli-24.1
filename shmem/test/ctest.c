#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rtai_shm.h"

#define MEMSIZE 3000

int main(void)
{
	unsigned int *adr, *adr1;
	printf("\nALLOCATING %x AND %x IN CURRENT PROCESS\n", 0xabcd, 0xaaaa);
	adr = rtai_malloc(0xabcd, 4*MEMSIZE);
	adr1 = rtai_malloc(0xaaaa, 1);
	rtai_malloc(0xaaaa, 1);
	rtai_malloc(0xaaaa, 1);
	rtai_malloc(0xaabc, 1);
	printf("THE FIRST VALUES OF %x AND %x ARE %d %d\n", 0xabcd, 0xaaaa, adr[0], adr1[0]);
	adr[0] = adr1[0] = 999999;
	printf("WE CHANGE THEM TO %d\n", adr[0]);
	rtai_check(0xabcd);
	printf("THE MODULE CHANGED THEM TO %d %d\n", adr[0], adr1[0]);
	printf("\nFREEING %x AND %x IN CURRENT PROCESS\n", 0xabcd, 0xaaaa);
	rtai_free(0xabcd, adr);
	rtai_free(0xaaaa, adr1);
	rtai_free(0xabcd, adr);
	rtai_free(0xffff, adr);
	return 0;
}
