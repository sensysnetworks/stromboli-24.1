/*
 * RTAI module to test RX from the serial line using callback functions.
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
MODULE_DESCRIPTION("Simple Serial Port RX Test using callback function\n");
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

static int msglen = 32; 
MODULE_PARM(msglen, "i");
MODULE_PARM_DESC(msglen, "Message length (def=32)");

static int rx_threshold = 0;
MODULE_PARM(rx_threshold, "i");
MODULE_PARM_DESC(rx_tx_thrs, "RX threshold for callback function (def=msglen)");

static int roundtrip = 0;
MODULE_PARM(roundtrip, "i");
MODULE_PARM_DESC(roundtrip, "Send message for round-trip test (def=0)");

static int debug = 1;
MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Print debug messages modulo <debug> (def=1)");

static char test_name[] = "test-rx-thrs";

#define STACK_SIZE 4000
 
RT_TASK thread;

#define MSGLEN_MAX 512
static char buf[MSGLEN_MAX];

static int nrx;

static void error_callback(int errors)
{
    rt_printk("%s: Line error(s) detected: 0x%x\n", test_name, errors);
}

static void threshold_callback(int rx_avbs, int tx_frbs)
{
    int j, rc;

    if (debug && !(nrx % debug))
	rt_printk("%s%d: threshold_callback(%d,%d)\n", 
		  test_name, rt_port, rx_avbs, tx_frbs);

    while (rx_avbs >= msglen) {
	
	if ((rc = rt_spread(rt_port, buf, msglen)))
	    rt_printk("RX%d failed (rc=0x%x)\n", rt_port, rc); 

	if (debug && !(nrx % debug)) {
	    buf[msglen] = '\0';
	    rt_printk("RX%d: %d received: '%s'\n", rt_port, nrx, buf);
	}

	if (roundtrip && tx_frbs >= msglen) {
	
	    /* Send round-trip message */
	    rc = sprintf(buf, "TX%d:%d:%d ", rt_port, nrx, msglen);
	    for (j = 0; j < msglen - rc - 1; j++)
		buf[j + rc] = '0' + j%10;
	    buf[msglen] = '\0';
	    
	    if ((rc = rt_spwrite(rt_port, buf, msglen)))
		rt_printk("TX%d failed (rc=0x%x)\n", rt_port, rc); 
	
	    if (debug && !(nrx % debug))
		rt_printk("TX%d: %d sent: '%s'\n", rt_port, nrx, buf);
	}

	nrx++;
	rx_avbs -= msglen;
    }
}

int init_module(void)
{
    int err;

    printk("%s: Loading RX%d Test module...\n", test_name, rt_port);

    /* Check and initialize some parameters */
    if (msglen > MSGLEN_MAX)
	msglen = MSGLEN_MAX;
    memset(buf, 0, MSGLEN_MAX);

    if (!rx_threshold) 
	rx_threshold = msglen; 

    nrx = 0;

    err = rt_spopen(rt_port, 
		    line_param[0], line_param[1], line_param[2], 
		    line_param[3], line_param[4], line_param[5]);
    if (err) return err;
	
    /* Register RX, TX threshold callback */
    rt_spset_callback_fun(rt_port, threshold_callback, rx_threshold, 0);

    /* Register error callback */
    rt_spset_err_callback_fun(rt_port, error_callback);

    return 0;
}

void cleanup_module(void)
{
    rt_spclose(rt_port);
    printk("%s: RX%d Test unloaded.\n", test_name, rt_port);
}
