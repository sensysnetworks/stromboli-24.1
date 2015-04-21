#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <asm/rtai.h>

int init_module(void)
{
	extern int calibrate_8254(void);
	rt_mount_rtai();
        printk("\n*** '#define SETUP_TIME_8254 %d' (IN USE %d), you can do 'make stop' now ***\n\n", calibrate_8254(), SETUP_TIME_8254);
	rt_umount_rtai();
	return 0;
}

void cleanup_module(void) { }
