#include <linux/module.h>

#include <asm/rtai.h>

#define ETHIRQ 11

static int cnt;
static void handler(int irq)
{
#ifndef CONFIG_RTAI_ADEOS
	rt_disable_irq(ETHIRQ);
#endif
	rt_pend_linux_irq(ETHIRQ);
	++cnt;
	rt_printk(">>> # RTAIIRQ: %d %d %d\n", cnt, irq, ETHIRQ);
}

static void linux_post_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	rt_enable_irq(ETHIRQ);
	rt_printk(">>> # LINUXIRQ: %d %d %d\n", cnt, irq, ETHIRQ);
}

int init_module(void)
{
	rt_request_linux_irq(ETHIRQ, linux_post_handler, "LINUX_POST_HANDLER", linux_post_handler);
	rt_request_global_irq(ETHIRQ, (void *)handler);
	rt_enable_irq(ETHIRQ);
	return 0;
}

void cleanup_module(void)
{
	rt_free_global_irq(ETHIRQ);
	rt_free_linux_irq(ETHIRQ, linux_post_handler);
	rt_enable_irq(ETHIRQ);
	printk("\n>>> CNT = %d\n", cnt);
}
