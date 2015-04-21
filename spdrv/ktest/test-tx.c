/*
 * RTAI module to test TX to the serial line.
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
MODULE_DESCRIPTION("Serial port TX test modul.\n");
MODULE_LICENSE("GPL");

static int rt_port = 0; 
MODULE_PARM(rt_port, "i");
MODULE_PARM_DESC(rt_port, "Serial RT port number (default: 0)");

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
MODULE_PARM_DESC(loops, "Number of messages to be sent");

#define MSGLEN_MAX 256
static int msglen = 32; 
MODULE_PARM(msglen, "i");
MODULE_PARM_DESC(msglen, "Message length");

static int period = 100000000;
MODULE_PARM(period, "i");
MODULE_PARM_DESC(period, "Tick and TX period in ns (def=10000000)");

static int timeout = 1000000000;
MODULE_PARM(timeout, "i");
MODULE_PARM_DESC(timeout, "RX/TX timeout in ns (def=1000000000)");

static int roundtrip = 0;
MODULE_PARM(roundtrip, "i");
MODULE_PARM_DESC(roundtrip, "Receive message for round-trip testing");

static int debug = 1;
MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Print debug messages modulo <debug>");

static char test_name[] = "test-tx";

#define STACK_SIZE 4000
 
RT_TASK thread;

static void error_handler(int errors)
{
    rt_printk("%s: Line error(s) detected: 0x%x\n", test_name, errors);
}

static void tx_handler(int dummy)
{
    char buf[MSGLEN_MAX];
    RTIME timeo, tstart;
    unsigned int timing, dt, dt_sum, dt_min, dt_max;
    int i, j, rc;

    /* Check parameters */
    if (msglen > MSGLEN_MAX)
	msglen = MSGLEN_MAX;
    memset(buf, 0, MSGLEN_MAX);

    timeo = nano2count(timeout);

    if (roundtrip && !debug) {
	timing = 1;
	dt_min = 2000000000;
	dt_max = 0;
	dt_sum = 0;
    } else {
	timing = 0;
    }

    for (i = 0; i < loops; i++) {

	/* Await next period */
	if (period)
	    rt_task_wait_period();
	
	/* Fill message with some readable text */
	rc = sprintf(buf, "TX%d:%d:%d ", rt_port, msglen, i);
	for (j = 0; j < msglen - rc - 1; j++)
	    buf[j + rc] = '0' + j%10;
	buf[msglen] = '\0';

	/* Get start time if selected */
	if (timing)
	    tstart = rt_get_time();

	/* Send Message */
	while ((rc = rt_spwrite_timed(rt_port, buf, msglen, timeo))) {
	    rt_printk("TX timeout (rc=0x%x)\n", rc);
	}
	
	if (debug && !(i % debug))
	    rt_printk("TX%d: %d sent: '%s'\n", rt_port, i, buf);

	/* Continue for roundtrip test */
	if (!roundtrip) continue;

	/* Receive message from round-trip test */
	while ((rc = rt_spread_timed(rt_port, buf, msglen, timeo))) {
	    rt_printk("RX%d timeout (rc=0x%x)\n", rt_port, rc);
	}
	
	/* Calculate round-trip times */
	if (timing) {
	    dt = (unsigned int)count2nano(rt_get_time() - tstart) / 1000;
	    if (dt < dt_min) dt_min = dt;
	    if (dt > dt_max) dt_max = dt;
	    dt_sum += dt;
	}

	if (debug && !(i % debug)) {
	    buf[msglen] = '\0';
	    rt_printk("RX%d: %d received: '%s'\n", rt_port, i, buf);
	}
    }
    
    if (timing) {
	rt_printk("%s: Roundtrip time: min=%dus max=%dus mean=%dus\n",
		  test_name, dt_min, dt_max, dt_sum / loops);
    }

    rt_printk("%s: TX%d handler terminated normally.\n", 
	      test_name, rt_port);
}

int init_module(void)
{
    RTIME tick_period;
    int err;

    printk("%s: Loading TX%d Test module...\n", test_name, rt_port);

    err = rt_spopen(rt_port, 
		    line_param[0], line_param[1], line_param[2], 
		    line_param[3], line_param[4], line_param[5]);
    if (err) return err;
	
    /* Register error handler */
    rt_spset_err_callback_fun(rt_port, error_handler);

    /* Start TX message handler */
    rt_task_init(&thread, tx_handler, 0, STACK_SIZE, 0, 0, 0);
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
    printk("%s: TX%d Test unloaded.\n", test_name, rt_port);
}
