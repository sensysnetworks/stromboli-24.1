#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/wrapper.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/io.h>

#include <rtai_shm.h>

#define SIZE 5000

int init_module (void)
{
	void *adr;
	adr = rtai_kmalloc(0xaaaa, SIZE);
	memset(adr, 255, SIZE);
	return 0;
}

void cleanup_module (void)
{
	rtai_kfree(0xaaaa);
	return;
}
