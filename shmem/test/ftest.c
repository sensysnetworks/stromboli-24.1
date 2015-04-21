#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rtai_shm.h"

int main(void)
{
	rtai_make_closable();
	return 0;
}
