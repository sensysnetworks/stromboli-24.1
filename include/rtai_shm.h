/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)
COPYRIGHT (C) 2001  Lineo Inc. (Author: bkuhn@lineo.com)

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

/*
ACKNOWLEDGMENTS:
- The suggestion and the code for mmapping at a user specified addres is due to
  Trevor Woolven (trevw@zentropix.com).
*/


#ifndef _RTAI_SHM_H_
#define _RTAI_SHM_H_

/*
 * This is so nasty.  The nommu interface and the mmu interface need
 * to be merged.
 */
#include <linux/config.h>

#ifndef CONFIG_UCLINUX
/*
 * MMU part
 */

#include <asm/rtai_shm.h>
#include <rtai_nam2num.h>
#ifndef __KERNEL__
#include <sys/ioctl.h>
#include <asm/page.h>
#endif

#define SHM_ADR(x)   ((void **)(&(x)))[0]
#define SHM_SIZE(x)  ((int *)(&(x)))[1]
#define SHM_COUNT(x) ((int *)(&(x)))[0]
#define REAL_SIZE(x) (((x) - 1) & PAGE_MASK) + PAGE_SIZE

extern void *rtai_kmalloc_f(int name, int size, unsigned long pid);
#define rtai_kmalloc(x, y)  rtai_kmalloc_f((x), (y), (unsigned long)(&(__this_module)))

extern void rtai_kfree_f(int name, unsigned long pid);
#define rtai_kfree(x)    rtai_kfree_f((x), (unsigned long)(&(__this_module)))

#ifndef __KERNEL__

#include <unistd.h>
#include <sys/mman.h>

#ifndef __SHM_USE_VECTOR
static inline long long rtai_shmrq(int srq, unsigned int whatever)
{
	int fd;
	int ret;

	fd = open("/dev/rtai_shm",O_RDWR);
	if(fd<0)return fd;

	ret = ioctl(fd,srq,whatever);

	close(fd);
	return ret;
}
#endif


static void *rtai_malloc(unsigned long name, int size)
{
	void *adr = 0;
	int hook;
	struct { unsigned long name, size; } arg;

	if (size <= 0) {
		return 0;
	}

	if ((hook = open("/dev/rtai_shm", O_RDWR)) < 0) {
		return 0;
	}
	arg.name = name;
	arg.size = size;

	if (!(size = rtai_shmrq(1, (unsigned long)(&arg)))) goto out;
	adr = mmap(NULL, size, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_FILE, hook, 0);
	if (adr == (void *)-1) return 0;

#ifdef DEBUG
	if ((size = rtai_shmrq(4, name)) < 0) {
		printf("REAL ALLOCATION OF %d BYTES FOR %lx.\n", -size, name);
	} else {
		printf("JUST A REMAP OF %d BYTES FOR %lx.\n", size, name);
	}
#endif

out:
	close(hook);

	return adr;
}

static inline void *rtai_malloc_adr(void *start_address, unsigned long name, int size)
{
	void *adr = 0;
	int hook;
	struct { unsigned long name, size; } arg;

	if (size <= 0) {
		return 0;
	}
	if ((hook = open("/dev/rtai_shm", O_RDWR)) < 0) goto out;

	arg.name = name;
	arg.size = size;
	if (!(size = ioctl(hook, 1, (unsigned long)(&arg)))) goto out;

	adr = mmap(start_address, size, PROT_WRITE|PROT_READ, 
		MAP_FIXED|MAP_SHARED|MAP_FILE, hook, 0);

out:
	close(hook);
	return adr;
}

static inline void rtai_free(int name, void *adr)
{
	int size;
	if ((size = rtai_shmrq(2, name))) {
#ifdef DEBUG
		{       
			int lsize;
			if ((lsize = rtai_shmrq(4, name)) < 0) {
				printf("FREED %d BYTES OF %lx.\n", -lsize, name);
			} else {
				printf("UNMAPPED %d BYTES OF %lx.\n", lsize, name);
			}       
		}       
#endif
		munmap(adr, size);
		rtai_shmrq(3, name);
	}
}

#ifdef DEBUG
static inline void rtai_check(unsigned long name)
{
	rtai_shmrq(5, name);
}

static inline int rtai_is_closable(void)
{
	return rtai_shmrq(6, 0);
}

static inline void rtai_make_closable(void)
{
	rtai_shmrq(7, 0);
}

static inline void rtai_not_closable(void)
{
	rtai_shmrq(8, 0);
}
#endif /* DEBUG */

#endif /* __KERNEL__ */

#else /* !CONFIG_UCLINUX */
/*
 * The non-mmu part
 */

#include <asm/rtai_shm.h>
#include <rtai_nam2num.h>

#define CMD_RTAI_KMALLOC 1
#define CMD_RTAI_KFREE 2

struct rtai_kmalloc_desc { unsigned long name; unsigned long size; };
struct rtai_kfree_desc { unsigned long name; void* addr; };

#ifndef __KERNEL__

static inline void *rtai_malloc(unsigned long name, int size) {
  struct rtai_kmalloc_desc arg = { name, size };
  return (void*)rtai_shmrq(CMD_RTAI_KMALLOC,(int)&arg);
};

static inline void rtai_free(int name, void *addr) {
  struct rtai_kfree_desc arg = { name, addr };
  rtai_shmrq(CMD_RTAI_KFREE,(int)&arg);
};

static inline void *rtai_malloc_adr(void *start_address, unsigned long name, int size) {
  printf("error: rtai_malloc_adr not available for MMU-less systems\n");
};

#else

extern void *rtai_kmalloc(int name, int size);
extern void rtai_kfree(int name);

#endif

#endif

#endif

