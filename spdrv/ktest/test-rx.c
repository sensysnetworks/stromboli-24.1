/*
 * RTAI module to test RX from the serial line.
 */

#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <asm/system.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/atomic.h>

#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>

#include <rtai_spdrv.h>

MODULE_AUTHOR("Wolfgang Grandegger");
MODULE_DESCRIPTION("Simple Serial Port RX Test.\n");
MODULE_LICENSE("GPL");

static int rt_port = 0; 
MODULE_PARM(rt_port, "i");
MODULE_PARM_DESC(rt_port, "Serial RT port number (def=0)");

static int line_param[6] = {
    115200,			/* Baud rate */
    8,				/* Bits per character */
    1,				/* Number of stop bits */
    RT_SP_PARITY_NONE,		/* Parity mode */
    RT_SP_NO_HAND_SHAKE,	/* Handshake mode */
    RT_SP_FIFO_SIZE_8		/* Fifo trigger level */
};
MODULE_PARM(line_param, "6i");
MODULE_PARM_DESC(line_param, "baudrate,wordlength,stopbits,parity,handshake,fifotrig");

static int loops = 10; 
MODULE_PARM(loops, "i");
MODULE_PARM_DESC(loops, "Number of messages to be received");

static int msglen = 32; 
MODULE_PARM(msglen, "i");
MODULE_PARM_DESC(msglen, "Message length (def=32)");

static int period = 0;
MODULE_PARM(period, "i");
MODULE_PARM_DESC(period, "Tick period in ns (def=0)");

static int timeout = 1000000000;
MODULE_PARM(timeout, "i");
MODULE_PARM_DESC(timeout, "RX/TX timeout in ns (def=1000000000)");

static int roundtrip = 0;
MODULE_PARM(roundtrip, "i");
MODULE_PARM_DESC(roundtrip, "Send message for round-trip test (def=0)");

static int debug = 1;
MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Print debug messages modulo <debug> (def=1)");

static char test_name[] = "test-rx";

#define STACK_SIZE 4000
 
RT_TASK thread;

#define MSGLEN_MAX 512
static char buf[MSGLEN_MAX];

static void error_handler(int errors)
{
    rt_printk("%s: Line error(s) detected: 0x%x\n", test_name, errors);
}

static void rx_handler(int dummy)
{
    RTIME timeo;
    int i, j, rc;

    /* Check parameters */
    if (msglen >= MSGLEN_MAX)
	msglen = MSGLEN_MAX - 1;
    memset(buf, 0, MSGLEN_MAX);

    timeo = nano2count(timeout);
	
    for (i = 0; i < loops; i++) {

	/* Receive Message */
	while ((rc = rt_spread_timed(rt_port, buf, msglen, timeo))) {
	    rt_printk("RX%d timeout (rc=0x%x)\n", rt_port, rc);
	}
	if (debug && !(i % debug)) {
	    buf[msglen] = '\0';
	    rt_printk("RX%d: %d received: '%s'\n", rt_port, i, buf);
	}
	if (!roundtrip) continue; 

	/* Send round-trip message */
	rc = sprintf(buf, "TX%d:%d:%d ", rt_port, msglen, i);
	for (j = 0; j < msglen - rc - 1; j++)
	    buf[j + rc] = '0' + j%10;
	buf[msglen] = '\0';

	/* Await next period */
	if (period)
	    rt_task_wait_period();

	while ((rc = rt_spwrite_timed(rt_port, buf, msglen, timeo))) {
	    rt_printk("TX%d timeout (rc=0x%x)\n", rt_port, rc);
	}
	if (debug && !(i % debug))
	    rt_printk("TX%d: %d sent: '%s'\n", rt_port, i, buf);

    }
    rt_printk("%s: RX%d handler terminated normally.\n", test_name, rt_port);
}

int init_module(void)
{
    RTIME tick_period;
    int err;

    printk("%s: Loading RX%d Test module...\n", test_name, rt_port);

    err = rt_spopen(rt_port, 
		    line_param[0], line_param[1], line_param[2], 
		    line_param[3], line_param[4], line_param[5]);
    if (err) return err;
	
    /* Register error handler */
    rt_spset_err_callback_fun(rt_port, error_handler);

    /* Start RX message handler */
    rt_task_init(&thread, rx_handler, 0, STACK_SIZE, 0, 0, 0);
    rt_task_resume(&thread);

    rt_set_oneshot_mode();
    if (period) {
	tick_period = start_rt_timer(nano2count(period));
	rt_task_make_periodic(&thread, rt_get_time(), tick_period);
    }
    return 0;
}

void cleanup_module(void)
{
    if (period)
	stop_rt_timer();
    rt_task_delete(&thread);
    rt_spclose(rt_port);
    printk("%s: RX%d Test unloaded.\n", test_name, rt_port);
}
