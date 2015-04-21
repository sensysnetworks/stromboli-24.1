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

#include <rtai_fifos.h>

#define FIFONUM 0
#define FIFOSIZE 4096

void *ptr1;

void do_cmd(int cmd)
{
	printk("fifos_mod: cmd=%d\n",cmd);
	switch(cmd){
	case 0:
		//printk("ptr1 = %p\n",ptr1);
		break;
	case 1:
		break;
	case 2:
		break;
	}
}

int handler(unsigned int fifo,int rw)
{
	int x;

	switch(rw){
	case 'r':
		break;
	case 'w':
		rtf_get(FIFONUM,&x,sizeof(x));
		do_cmd(x);
		break;
	}
	return 0;
}

int init_module(void)
{
	rtf_create(FIFONUM,FIFOSIZE);
	rtf_create_handler(FIFONUM,X_FIFO_HANDLER(handler));

	return 0;
}

void cleanup_module(void)
{
	rtf_destroy(FIFONUM);
	return;
}

