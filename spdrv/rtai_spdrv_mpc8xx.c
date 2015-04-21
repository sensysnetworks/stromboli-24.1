/*
 * Copyright (c) 2004  Wolfgang Grandegger (wg@denx.de)
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 * 
 * Derived from rtai-24.1.12/spdrv/spdrv.[ch]:
 *  COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)
 *                      Giuseppe Renoldi (giuseppe@renoldi.org)
 * and from linuxppc_2_4_devel/arch/ppc/8xx_io/uart.c:
 *  UART driver for MPC8xx CPM SCC or SMC (CPM UART drivers 0.04)
 *  Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *  Copyright (c) 2001 Wolfgang Denk (wd@denx.de) [buffer config, HW handshake]
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <asm/uaccess.h> 
#include <asm/system.h>
#include <asm/io.h>
#include <asm/8xx_immap.h>
#include <asm/mpc8xx.h>
#include <asm/irq.h>
#include <asm/commproc.h>

#include <rtai.h>
#include <rtai_sched.h>

#include "rtai_spdrv.h"
#include "rtai_spdrv_mpc8xx.h"

MODULE_AUTHOR("Wolfgang Grandegger");
MODULE_DESCRIPTION("RTAI real time MPC8xx serial port driver");
MODULE_LICENSE("GPL");

static int spbufsiz = SPBUFSIZ;
MODULE_PARM(spbufsiz, "i");
MODULE_PARM_DESC(spbufsize, "Size of RX/TX buffer");

static int spprio[6] = {SPPRIO,SPPRIO,SPPRIO,SPPRIO,SPPRIO,SPPRIO};
MODULE_PARM(spprio, "1-" __MODULE_STRING(6) "i");
MODULE_PARM_DESC(spprio, "RTAI priority of the service task(s)");

/*
 * Debug definitions
 */
//#define DEBUG_SPDRV
#ifdef DEBUG_SPDRV
# define debugk(fmt,args...) printk(fmt ,##args)
#else
# define debugk(fmt,args...)
#endif

//#define RT_DEBUG_SPDRV
#ifdef RT_DEBUG_SPDRV
# define rt_debugk(fmt,args...) rt_printk(fmt ,##args)
#else
# define rt_debugk(fmt,args...)
#endif

/*
 * Serial port can be used either by RTAI or by Linux!
 */
#if defined(CONFIG_SMC1_UART) && defined(RTAI_SMC1_UART)
#error "SMC1 UART already used by Linux kernel, please correct!"
#endif
#if defined(CONFIG_SMC2_UART) && defined(RTAI_SMC2_UART)
#error "SMC2 UART already used by Linux kernel, please correct!"
#endif
#if defined(CONFIG_SCC1_UART) && defined(RTAI_SCC1_UART)
#error "SCC1 UART already used by Linux kernel, please correct!"
#endif
#if defined(CONFIG_SCC2_UART) && defined(RTAI_SCC2_UART)
#error "SCC2 UART already used by Linux kernel, please correct!"
#endif
#if defined(CONFIG_SCC3_UART) && defined(RTAI_SCC3_UART)
#error "SCC3 UART already used by Linux kernel, please correct!"
#endif
#if defined(CONFIG_SCC4_UART) && defined(RTAI_SCC4_UART)
#error "SCC4 UART already used by Linux kernel, please correct!"
#endif

#if defined(RTAI_UART_CTS_CONTROL_SCC1) || \
    defined(RTAI_UART_RTS_CONTROL_SCC1) || \
    defined(RTAI_UART_CD_CONTROL_SCC1)  || \
    defined(RTAI_UART_DTR_CONTROL_SCC1) || \
    defined(RTAI_UART_CTS_CONTROL_SCC2) || \
    defined(RTAI_UART_RTS_CONTROL_SCC2) || \
    defined(RTAI_UART_CD_CONTROL_SCC2)  || \
    defined(RTAI_UART_DTR_CONTROL_SCC2) || \
    defined(RTAI_UART_CTS_CONTROL_SCC3) || \
    defined(RTAI_UART_RTS_CONTROL_SCC3) || \
    defined(RTAI_UART_CD_CONTROL_SCC3)  || \
    defined(RTAI_UART_DTR_CONTROL_SCC3) || \
    defined(RTAI_UART_CTS_CONTROL_SCC4) || \
    defined(RTAI_UART_RTS_CONTROL_SCC4) || \
    defined(RTAI_UART_CD_CONTROL_SCC4)  || \
    defined(RTAI_UART_DTR_CONTROL_SCC4)
#define CPM_UART_HANDSHAKING
#endif

#ifdef CPM_UART_HANDSHAKING
#define PORT_A 1
#define PORT_B 2
#define PORT_C 3
#define PORT_D 4

#if (RTAI_UART_DTR_CONTROL_SCC1 == PORT_A)
#define PORT_DTR1_PAR immap->im_ioport.iop_papar
#define PORT_DTR1_DAT immap->im_ioport.iop_padat
#define PORT_DTR1_DIR immap->im_ioport.iop_padir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC1 == PORT_B)
#define PORT_DTR1_PAR immap->im_cpm.cp_pbpar
#define PORT_DTR1_DAT immap->im_cpm.cp_pbdat
#define PORT_DTR1_DIR immap->im_cpm.cp_pbdir
#define DTR1_PIN (1 << (31 - RTAI_DTR1_PIN))
#endif
#if (RTAI_UART_DTR_CONTROL_SCC1 == PORT_C)
#define PORT_DTR1_PAR immap->im_ioport.iop_pcpar
#define PORT_DTR1_DAT immap->im_ioport.iop_pcdat
#define PORT_DTR1_DIR immap->im_ioport.iop_pcdir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC1 == PORT_D)
#define PORT_DTR1_PAR immap->im_ioport.iop_pdpar
#define PORT_DTR1_DAT immap->im_ioport.iop_pddat
#define PORT_DTR1_DIR immap->im_ioport.iop_pddir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC2 == PORT_A)
#define PORT_DTR2_PAR immap->im_ioport.iop_papar
#define PORT_DTR2_DAT immap->im_ioport.iop_padat
#define PORT_DTR2_DIR immap->im_ioport.iop_padir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC2 == PORT_B)
#define PORT_DTR2_PAR immap->im_cpm.cp_pbpar
#define PORT_DTR2_DAT immap->im_cpm.cp_pbdat
#define PORT_DTR2_DIR immap->im_cpm.cp_pbdir
#define DTR2_PIN (1 << (31 - RTAI_DTR2_PIN))
#endif
#if (RTAI_UART_DTR_CONTROL_SCC2 == PORT_C)
#define PORT_DTR2_PAR immap->im_ioport.iop_pcpar
#define PORT_DTR2_DAT immap->im_ioport.iop_pcdat
#define PORT_DTR2_DIR immap->im_ioport.iop_pcdir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC2 == PORT_D)
#define PORT_DTR2_PAR immap->im_ioport.iop_pdpar
#define PORT_DTR2_DAT immap->im_ioport.iop_pddat
#define PORT_DTR2_DIR immap->im_ioport.iop_pddir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC3 == PORT_A)
#define PORT_DTR3_PAR immap->im_ioport.iop_papar
#define PORT_DTR3_DAT immap->im_ioport.iop_padat
#define PORT_DTR3_DIR immap->im_ioport.iop_padir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC3 == PORT_B)
#define PORT_DTR3_PAR immap->im_cpm.cp_pbpar
#define PORT_DTR3_DAT immap->im_cpm.cp_pbdat
#define PORT_DTR3_DIR immap->im_cpm.cp_pbdir
#define DTR3_PIN (1 << (31 - RTAI_DTR3_PIN))
#endif
#if (RTAI_UART_DTR_CONTROL_SCC3 == PORT_C)
#define PORT_DTR3_PAR immap->im_ioport.iop_pcpar
#define PORT_DTR3_DAT immap->im_ioport.iop_pcdat
#define PORT_DTR3_DIR immap->im_ioport.iop_pcdir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC3 == PORT_D)
#define PORT_DTR3_PAR immap->im_ioport.iop_pdpar
#define PORT_DTR3_DAT immap->im_ioport.iop_pddat
#define PORT_DTR3_DIR immap->im_ioport.iop_pddir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC4 == PORT_A)
#define PORT_DTR4_PAR immap->im_ioport.iop_papar
#define PORT_DTR4_DAT immap->im_ioport.iop_padat
#define PORT_DTR4_DIR immap->im_ioport.iop_padir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC4 == PORT_B)
#define PORT_DTR4_PAR immap->im_cpm.cp_pbpar
#define PORT_DTR4_DAT immap->im_cpm.cp_pbdat
#define PORT_DTR4_DIR immap->im_cpm.cp_pbdir
#define DTR4_PIN (1 << (31 - RTAI_DTR4_PIN))
#endif
#if (RTAI_UART_DTR_CONTROL_SCC4 == PORT_C)
#define PORT_DTR4_PAR immap->im_ioport.iop_pcpar
#define PORT_DTR4_DAT immap->im_ioport.iop_pcdat
#define PORT_DTR4_DIR immap->im_ioport.iop_pcdir
#endif
#if (RTAI_UART_DTR_CONTROL_SCC4 == PORT_D)
#define PORT_DTR4_PAR immap->im_ioport.iop_pdpar
#define PORT_DTR4_DAT immap->im_ioport.iop_pddat
#define PORT_DTR4_DIR immap->im_ioport.iop_pddir
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC1
#if (RTAI_UART_DTR_CONTROL_SCC1 != PORT_B)
#define DTR1_PIN (1 << (15 - RTAI_DTR1_PIN))
#endif
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC2
#if (RTAI_UART_DTR_CONTROL_SCC2 != PORT_B)
#define DTR2_PIN (1 << (15 - RTAI_DTR2_PIN))
#endif
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC3
#if (RTAI_UART_DTR_CONTROL_SCC3 != PORT_B)
#define DTR3_PIN (1 << (15 - RTAI_DTR3_PIN))
#endif
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC4
#if (RTAI_UART_DTR_CONTROL_SCC4 != PORT_B)
#define DTR4_PIN (1 << (15 - RTAI_DTR4_PIN))
#endif
#endif

#endif	/* CPM_UART_HANDSHAKING */

/* We overload some of the items in the data structure to meet our
 * needs.  For example, the port address is the CPM parameter ram
 * offset for the SCC or SMC.  The maximum number of ports is 4 SCCs and
 * 2 SMCs.  The "hub6" field is used to indicate the channel number, with
 * a flag indicating SCC or SMC, and the number is used as an index into
 * the CPM parameter area for this device.
 * The "type" field is currently set to 0, for PORT_UNKNOWN.  It is
 * not currently used.  I should probably use it to indicate the port
 * type of SMC or SCC.
 * The SMCs do not support any modem control signals.
 */
#define smc_scc_num	     hub6
#define NUM_IS_SCC	     ((int)0x00010000)
#define NUM_BRG              ((int)0x0000FF00)
#define NUM_BRG_SHIFT        8
#define NUM                  ((int)0x000000FF)
#define NUM_SHIFT            0
#define PORT_NUM(P)	     ((P) & NUM)
#define PORT_NUM_SET(N)      (((N)-1) << NUM_SHIFT)
#define PORT_IS_SCC(P)	     ((P) & NUM_IS_SCC)
#define PORT_BRG(P)          (((P) & NUM_BRG) >> NUM_BRG_SHIFT)
#define PORT_BRG_SET(P,B)    (P) = (((P) & ~NUM_BRG) | ((B) << NUM_BRG_SHIFT))

/* Short names for the ports
*/
#define QUICC_CPM_SMC1  (PORT_NUM_SET(1))
#define QUICC_CPM_SMC2  (PORT_NUM_SET(2))
#define QUICC_CPM_SCC1  (PORT_NUM_SET(1)|NUM_IS_SCC)
#define QUICC_CPM_SCC2  (PORT_NUM_SET(2)|NUM_IS_SCC)
#define QUICC_CPM_SCC3  (PORT_NUM_SET(3)|NUM_IS_SCC)
#define QUICC_CPM_SCC4  (PORT_NUM_SET(4)|NUM_IS_SCC)
#define QUICC_MAX_BRG   3  /* BRG1..BRG4 */

/* Processors other than the 860 only get SMCs configured by default.
 * Either they don't have SCCs or they are allocated somewhere else.
 * Of course, there are now 860s without some SCCs, so we will need to
 * address that someday.
 * The Embedded Planet Multimedia I/O cards use TDM interfaces to the
 * stereo codec parts, and we use SMC2 to help support that.
 */
struct rt_spstate {
        int     magic;
        int     baud_base;
        unsigned long   port;
        int     irq;
        int     flags;
        int     hub6;
        int     type;
        int     line;
};

static struct rt_spstate rs_table[] = {
	/* UART CLK   PORT          IRQ      FLAGS  NUM   */
#ifdef RTAI_SMC1_UART
  	{ 0,     0, PROFF_SMC1, CPMVEC_SMC1,   0,    QUICC_CPM_SMC1 },    /* SMC1 ttySx */
#endif
#ifdef RTAI_SMC2_UART
  	{ 0,     0, PROFF_SMC2, CPMVEC_SMC2,   0,    QUICC_CPM_SMC2 },    /* SMC2 ttySx */
#endif
#ifdef RTAI_SCC1_UART
  	{ 0,     0, PROFF_SCC1, CPMVEC_SCC1,   0,    QUICC_CPM_SCC1 },    /* SCC1 ttySx */
#endif
#ifdef RTAI_SCC2_UART
  	{ 0,     0, PROFF_SCC2, CPMVEC_SCC2,   0,    QUICC_CPM_SCC2 },    /* SCC2 ttySx */
#endif
#ifdef RTAI_SCC3_UART
  	{ 0,     0, PROFF_SCC3, CPMVEC_SCC3,   0,    QUICC_CPM_SCC3 },    /* SCC3 ttySx */
#endif
#ifdef RTAI_SCC4_UART
  	{ 0,     0, PROFF_SCC4, CPMVEC_SCC4,   0,    QUICC_CPM_SCC4 },    /* SCC4 ttySx */
#endif
};

#define NR_PORTS	(sizeof(rs_table)/sizeof(struct rt_spstate))

static int rx_bd_nums[6] = {
#ifdef RTAI_SCC1_UART_RX_BDNUM
  RTAI_SCC1_UART_RX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SCC2_UART_RX_BDNUM
  RTAI_SCC2_UART_RX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SCC3_UART_RX_BDNUM
  RTAI_SCC3_UART_RX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SCC4_UART_RX_BDNUM
  RTAI_SCC4_UART_RX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SMC1_UART_RX_BDNUM
  RTAI_SMC1_UART_RX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SMC2_UART_RX_BDNUM
  RTAI_SMC2_UART_RX_BDNUM
#else
  0
#endif
};

static int tx_bd_nums[6] = {
#ifdef RTAI_SCC1_UART_TX_BDNUM
  RTAI_SCC1_UART_TX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SCC2_UART_TX_BDNUM
  RTAI_SCC2_UART_TX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SCC3_UART_TX_BDNUM
  RTAI_SCC3_UART_TX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SCC4_UART_TX_BDNUM
  RTAI_SCC4_UART_TX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SMC1_UART_TX_BDNUM
  RTAI_SMC1_UART_TX_BDNUM,
#else
  0,
#endif
#ifdef RTAI_SMC2_UART_TX_BDNUM
  RTAI_SMC2_UART_TX_BDNUM
#else
  0
#endif
};

static int rx_bd_sizes[6] = {
#ifdef RTAI_SCC1_UART_RX_BDSIZE
  RTAI_SCC1_UART_RX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SCC2_UART_RX_BDSIZE
  RTAI_SCC2_UART_RX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SCC3_UART_RX_BDSIZE
  RTAI_SCC3_UART_RX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SCC4_UART_RX_BDSIZE
  RTAI_SCC4_UART_RX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SMC1_UART_RX_BDSIZE
  RTAI_SMC1_UART_RX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SMC2_UART_RX_BDSIZE
  RTAI_SMC2_UART_RX_BDSIZE
#else
  0
#endif
};

static int tx_bd_sizes[6] = {
#ifdef RTAI_SCC1_UART_TX_BDSIZE
  RTAI_SCC1_UART_TX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SCC2_UART_TX_BDSIZE
  RTAI_SCC2_UART_TX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SCC3_UART_TX_BDSIZE
  RTAI_SCC3_UART_TX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SCC4_UART_TX_BDSIZE
  RTAI_SCC4_UART_TX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SMC1_UART_TX_BDSIZE
  RTAI_SMC1_UART_TX_BDSIZE,
#else
  0,
#endif
#ifdef RTAI_SMC2_UART_TX_BDSIZE
  RTAI_SMC2_UART_TX_BDSIZE
#else
  0
#endif
};

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#define buf_atomic_bgn(flags, mbx) \
	do { flags = rt_spin_lock_irqsave(&(mbx)->lock); } while (0)
#define buf_atomic_end(flags, mbx) \
	do { rt_spin_unlock_irqrestore(flags, &(mbx)->lock); } while (0)


struct rt_spmbx {
	int frbs, avbs, fbyte, lbyte;
	char *bufadr;
	spinlock_t lock;
};

typedef struct {
	/* CPM Buffer Descriptor sizes and pointers.
	 */
	short rx_bd_size;
	cbd_t *rx_bd_base;
	cbd_t *rx_cur;
	short tx_bd_size;
	cbd_t *tx_bd_base;
	cbd_t *tx_cur;

	/* Virtual addresses for the FIFOs because we can't __va() a
	 * physical address anymore.
	 */
	unsigned char *rx_va_base;
	unsigned char *tx_va_base;

	int opened;	
	int error;
	int just_onew, just_oner;
	SEM txsem, rxsem;
	SEM service_sem;
	RT_TASK service_thread;
	struct rt_spstate *state;
	spinlock_t lock;

	char name[8];
	struct rt_spmbx ibuf, obuf;
	void (*callback_fun)(int, int);
	volatile int rxthrs, txthrs;
	void (*err_callback_fun)(int);
	RT_TASK *callback_task;
	unsigned long callback_fun_usr;
	unsigned long err_callback_fun_usr;
	volatile unsigned long call_usr;
#ifdef IS_READY
	int read_status_mask;
	int ignore_status_mask;
#endif
	unsigned short tx_dp_addr;
	unsigned short rx_dp_addr;

} rt_spct_t;

rt_spct_t *spct;

static int spcnt;	// number of available serial ports

#define CHECK_SPINDX(indx)  do { if (indx >= spcnt) return -ENODEV; } while (0)
static inline rt_spct_t *get_spct(unsigned int tty) {
	if (tty < spcnt && spct[tty].opened)
		return &spct[tty];
	return NULL;
}

#define STACK_SIZE 4000


static void mbx_init(struct rt_spmbx *mbx)
{
	unsigned long flags;
	spin_lock_init(&mbx->lock);
	buf_atomic_bgn(flags, mbx);
	mbx->frbs = spbufsiz;
	mbx->fbyte = mbx->lbyte = mbx->avbs = 0;
	buf_atomic_end(flags, mbx);
}

#define MOD_SIZE(indx) ((indx) < spbufsiz ? (indx) : (indx) - spbufsiz)

static int mbxput(struct rt_spmbx *mbx, char *msg, int msg_size)
{
	unsigned long flags;
	int tocpy;
	char *m = msg;

	while (msg_size > 0 && mbx->frbs) {
		if ((tocpy = spbufsiz - mbx->lbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > mbx->frbs) {
			tocpy = mbx->frbs;
		}
		memcpy(mbx->bufadr + mbx->lbyte, m, tocpy);
		buf_atomic_bgn(flags, mbx);
		mbx->frbs -= tocpy;
		mbx->avbs += tocpy;
		buf_atomic_end(flags, mbx);
		msg_size  -= tocpy;
		m         += tocpy;
		mbx->lbyte = MOD_SIZE(mbx->lbyte + tocpy);
		rt_debugk("mbxput: avbs=%d frbs=%d\n", mbx->avbs, mbx->frbs);

	}
	return msg_size;
}

static int mbxget(struct rt_spmbx *mbx, char *msg, int msg_size)
{
	unsigned long flags;
	int tocpy;
	char *m = msg;

	while (msg_size > 0 && mbx->avbs) {
		if ((tocpy = spbufsiz - mbx->fbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > mbx->avbs) {
			tocpy = mbx->avbs;
		}
		memcpy(m, mbx->bufadr + mbx->fbyte, tocpy);
		buf_atomic_bgn(flags, mbx);
		mbx->frbs  += tocpy;
		mbx->avbs  -= tocpy;
		buf_atomic_end(flags, mbx);
		msg_size   -= tocpy;
		m          += tocpy;
		mbx->fbyte = MOD_SIZE(mbx->fbyte + tocpy);
	}
	return msg_size;
}

static inline int mbxevdrp(struct rt_spmbx *mbx, char *msg, int msg_size)
{
	int tocpy, fbyte, avbs;
	char *m = msg;

	fbyte = mbx->fbyte;
	avbs  = mbx->avbs;
	while (msg_size > 0 && avbs) {
		if ((tocpy = spbufsiz - fbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > avbs) {
			tocpy = avbs;
		}
		memcpy(m, mbx->bufadr + fbyte, tocpy);
		avbs     -= tocpy;
		msg_size -= tocpy;
		m        += tocpy;
		fbyte = MOD_SIZE(fbyte + tocpy);
	}
	return msg_size;
}


/*
 * rt_spclear_rx
 *
 * Clear all received chars in buffer and inside UART FIFO
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EACCES		if rx occupied
 *
 */ 
int rt_spclear_rx(unsigned int tty)
{
 	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (!test_and_set_bit(0, &p->just_oner)) {
		mbx_init(&p->ibuf);
		/* FIXME: do not know yet how to clean RX buffers in hardware */
		clear_bit(0, &p->just_oner);
		return 0;
	}
	return -EACCES;
}


/*
 * rt_spclear_tx
 *
 * Clear all chars in buffer to be transmitted and inside TX UART FIFO
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EACCES		if tx occupied
 *
 */ 
int rt_spclear_tx(unsigned int tty)
{
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (!test_and_set_bit(0, &p->just_onew)) {
		mbx_init(&p->obuf);
		clear_bit(0, &p->just_onew);
		return 0;
	}
	return -EACCES;
}


/*
 * rt_spset_mode
 *
 * Set the handshaking mode for serial line
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 * 		mode		RT_SP_NO_HAND_SHAKE 
 * 				RT_SP_DSR_ON_TX 	transmitter enabled if DSR active
 * 				RT_SP_HW_FLOW		RTS-CTS flow control
 * 					
 *				Note: RT_SP_DSR_ON_TX and RT_SP_HW_FLOW can
 *				      be ORed together 
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spset_mode(unsigned int tty, int mode)
{
	rt_printk("rt_spset_mode() is not supported.\n");
	return 0;
}


/*
 * rt_spset_fifotrig
 *
 * Set the trigger level for UART RX FIFO
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 *		fifotrig	RT_SP_FIFO_DISABLE
 *      			RT_SP_FIFO_SIZE_1
 *      			RT_SP_FIFO_SIZE_4
 *      			RT_SP_FIFO_SIZE_8
 *      			RT_SP_FIFO_SIZE_14
 *      			RT_SP_FIFO_SIZE_DEFAULT
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EINVAL		if wrong fifotrig value
 *
 */ 
int rt_spset_fifotrig(unsigned int tty, int fifotrig)
{
	rt_printk("rt_spset_fifotrig() is not supported.\n");
	return 0;
}


/*
 * rt_spset_mcr
 *
 * Set MODEM Control Register MCR
 * -> DTR, RTS bits
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 *		mask		RT_SP_DTR | RT_SP_RTS
 *      			
 *		setbits		0 -> reset bits in mask
 *      			1 -> set bits in mask
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EINVAL		if wrong mask value
 *
 */ 
int rt_spset_mcr(unsigned int tty, int mask, int setbits)
{
	rt_printk("rt_spset_mcr() is not supported.\n");
	return 0;
}


/*
 * rt_spget_msr
 *
 * Get MODEM Status Register MSR
 * -> CTS, DSR, RI, DCD 
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 *		mask		RT_SP_CTS | RT_SP_DSR | RT_SP_RI | RT_SP_DCD
 *      			
 * Return Value:
 * 		masked MSR register value
 * 		or
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spget_msr(unsigned int tty, int mask)
{
	rt_printk("rt_spget_mcr() is not supported.\n");
	return 0;
}


/*
 * rt_spget_err
 *
 * Return and reset last error detected by UART inside interrupt 
 * service routine
 *
 * Arguments: 
 * 		tty		serial port number
 *      			
 * Return Value:
 * 		last error condition code
 * 		
 * 		This condition code can be one of this values:
 * 		RT_SP_BUFFER_FULL
 * 		RT_SP_BUFFER_OVF
 * 		RT_SP_OVERRUN_ERR
 * 		RT_SP_PARITY_ERR
 * 		RT_SP_FRAMING_ERR
 * 		RT_SP_BREAK
 *
 */ 
int rt_spget_err(unsigned int tty)
{
	rt_spct_t *p;
   	int tmp;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	tmp = p->error;
	p->error = 0;
	return tmp;
}


/*
 * rt_spwrite
 *
 * Send one or more bytes 
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 * 		msg		pointer to the chars to send
 *
 *		msg_size	abs(msg_size) is the number of bytes to write.
 * 				If <0, write bytes only if possible to write them all together.
 *      			
 * Return Value:
 * 		number of chars NOT sent
 * 		or
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spwrite(unsigned int tty, char *msg, int msg_size)
{
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (!test_and_set_bit(0, &p->just_onew)) {
		if (msg_size > 0 || (msg_size = -msg_size) <= p->obuf.frbs) {
			msg_size = mbxput(&p->obuf, msg, msg_size);
			/* Wake up service task */
			rt_sem_signal(&p->service_sem);
		}
		clear_bit(0, &p->just_onew);
	}
	return msg_size;
}


/*
 * rt_spread
 *
 * Get one or more bytes from what received removing them
 * from buffer
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 * 		msg		pointer to the buffer where to put chars received
 *
 *		msg_size	abs(msg_size) is the number of bytes to read. 
 *				If >0, read all the bytes up to msg_size 
 *				If <0, read bytes only if possible to read them all together.
 *      			
 * Return Value:
 * 		number of chars NOT get (respect to what requested)
 * 		or
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spread(unsigned int tty, char *msg, int msg_size)
{
	rt_spct_t *p;
	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (!test_and_set_bit(0, &p->just_oner)) {
		if (msg_size > 0 || (msg_size = -msg_size) <= p->ibuf.avbs) {
			msg_size = mbxget(&p->ibuf, msg, msg_size);
		}
		clear_bit(0, &p->just_oner);
	}
	return msg_size;
}


/*
 * rt_spevdrp
 *
 * Get one or more bytes from what received WITHOUT removing them
 * from buffer
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 * 		msg		pointer to the buffer where to put chars received
 *
 *		msg_size	abs(msg_size) is the number of bytes to read. 
 *				If >0, read all the bytes up to msg_size 
 *				If <0, read bytes only if possible to read them all together.
 *      			
 * Return Value:
 * 		number of chars NOT get (respect to what requested)
 * 		or
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spevdrp(unsigned int tty, char *msg, int msg_size)
{
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (!test_and_set_bit(0, &p->just_oner)) {
		if (msg_size > 0 || (msg_size = -msg_size) <= p->ibuf.avbs) {
			msg_size = mbxevdrp(&p->ibuf, msg, msg_size);
		}
		clear_bit(0, &p->just_oner);
	}
	return msg_size;
}


/*
 * rt_spwrite_timed
 *
 * Send one or more bytes with timeout
 *
 * Arguments: 
 * 		tty		serial port number.
 * 		
 * 		msg		pointer to the chars to send.
 *
 *		msg_size	number of bytes to send.
 *
 *		delay		timeout in internal count unit,
 *				use DELAY_FOREVER for a blocking send.
 *      			
 * Return Value:
 * 		-ENODEV, if wrong tty number;
 * 		msg_size, if < 0 or another writer is already using tty;
 * 		one of the semaphores error messages;
 * 		0, message sent succesfully.
 *
 */ 
int rt_spwrite_timed(unsigned int tty, char *msg, int msg_size, RTIME delay)
{
	rt_spct_t *p;
	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (msg_size > 0) {
		if (!test_and_set_bit(0, &p->just_onew)) {
			(p->txsem).count = 0;
			p->txthrs = -msg_size;
			if (msg_size > p->obuf.frbs) {
				int semret;
				if ((semret = rt_sem_wait_timed(&p->txsem, delay))) {
					p->txthrs = 0;
					clear_bit(0, &p->just_onew);
					return semret;
				}
			} else {
				p->txthrs = 0;
			}
			mbxput(&p->obuf, msg, msg_size);
			/* Wake up service task */
			rt_sem_signal(&p->service_sem);
			clear_bit(0, &p->just_onew);
			return 0;
		}
	}
	return msg_size;
}


/*
 * rt_spread_timed
 *
 * Receive one or more bytes with timeout
 *
 * Arguments: 
 * 		tty		serial port number.
 * 		
 * 		msg		pointer to the chars to receive.
 *
 *		msg_size	the number of bytes to receive.
 *
 *		delay		timeout in internal count unit,
 *				use DELAY_FOREVER for a blocking receive.
 *      			
 * Return Value:
 * 		-ENODEV, if wrong tty number;
 * 		msg_size, if < 0 or another reader is already using tty;
 * 		one of the semaphores error messages;
 * 		0, message received succesfully.
 *
 */ 
int rt_spread_timed(unsigned int tty, char *msg, int msg_size, RTIME delay)
{
	rt_spct_t *p;
	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (msg_size > 0) {
		if (!test_and_set_bit(0, &p->just_oner)) {
			(p->rxsem).count = 0;
			p->rxthrs = -msg_size;
			if (msg_size > p->ibuf.avbs) {
				int semret;
				if ((semret = rt_sem_wait_timed(&p->rxsem, delay))) {
					p->txthrs = 0;
					clear_bit(0, &p->just_oner);
					return semret;
				}
			} else {
				p->txthrs = 0;
			}
			mbxget(&p->ibuf, msg, msg_size);
			clear_bit(0, &p->just_oner);
			return 0;
		}
	}
	return msg_size;
}


/*
 * rt_spget_rxavbs
 *
 * Get how many chars are in receive buffer
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 * Return Value:
 * 		number of chars in receive buffer
 * 		or
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spget_rxavbs(unsigned int tty)
{
	CHECK_SPINDX(tty);
	return spct[tty].ibuf.avbs;
}


/*
 * rt_spget_txfrbs
 *
 * Get how many chars are in transmit buffer waiting to be sent by UART
 *
 * Arguments: 
 * 		tty		serial port number
 * 		
 * Return Value:
 * 		number of chars in transmit buffer
 * 		or
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spget_txfrbs(unsigned int tty)
{
	CHECK_SPINDX(tty);
	return spct[tty].obuf.frbs;
}


static int rt_spisr(int irq, rt_spct_t *p)
{
	rt_debugk("%s: got IRQ%d\n", p->name, irq);
	rt_sem_signal(&p->service_sem);
	return 0;
}


static int sp8xx_receive_chars(rt_spct_t *p)
{
	char *cp;
	/*int	ignored = 0;*/
	int	i, nrx = 0;
	ushort	status;
	volatile cbd_t	*bdp;

	rt_debugk("%s: sp8xx_receive_chars...\n", p->name);

	/* Just loop through the closed BDs and copy the characters into
	 * the buffer.
	 */
	bdp = p->rx_cur;
	for (;;) {
		if (bdp->cbd_sc & BD_SC_EMPTY)	/* If this one is empty */
			break;			/*   we are all done */

		/* Get the number of characters and the buffer pointer.
		*/
		i = bdp->cbd_datlen;
		cp = p->rx_va_base + ((bdp - p->rx_bd_base) * p->rx_bd_size);
		status = bdp->cbd_sc;

		/* Check to see if there is room in the tty buffer for
		 * the characters in our BD buffer.  If not, we exit
		 * now, leaving the BD with the characters.  We'll pick
		 * them up again on the next receive interrupt (which could
		 * be a timeout).
		 */
		rt_debugk("%s: RX %d chars (avbs=%d frbs=%d), status=0x%x\n", 
			  p->name, i, p->ibuf.avbs, p->ibuf.frbs);
		if (i < p->ibuf.frbs) {
			nrx += i - mbxput(&p->ibuf, cp, i);
		} else {
			p->error |= RT_SP_BUFFER_OVF;
			break;
		}
		
		if (status & (BD_SC_BR | BD_SC_FR | BD_SC_PR | BD_SC_OV)) {
			if (status & BD_SC_BR) {
				p->error |= RT_SP_BREAK;
				rt_debugk("Break error (status=0x%x)\n", status);
			}
			else if (status & BD_SC_PR) {
				p->error |= RT_SP_PARITY_ERR;
				rt_debugk("Parity error (status=0x%x)\n", status);
			}
			else if (status & BD_SC_FR) {
				p->error |= RT_SP_FRAMING_ERR;
				rt_debugk("Framing error (status=0x%x)\n", status);
			}
			if (status & BD_SC_OV) {
				p->error |= RT_SP_OVERRUN_ERR;
				rt_debugk("Overrun error (status=0x%x)\n", status);
			}
		}
		
		/* This BD is ready to be used again.  Clear status.
		 * Get next BD.
		 */
		bdp->cbd_sc |= BD_SC_EMPTY;
		bdp->cbd_sc &= ~(BD_SC_BR | BD_SC_FR | BD_SC_PR | BD_SC_OV);

		if (bdp->cbd_sc & BD_SC_WRAP)
			bdp = p->rx_bd_base;
		else
			bdp++;
	}
	p->rx_cur = (cbd_t *)bdp;
	return nrx;
}


static int sp8xx_transmit_chars(rt_spct_t *p)
{
	int	c, ret = 0;
	volatile cbd_t *bdp;
	char	*cp;

	while (1) {
		c = MIN(p->obuf.avbs, p->tx_bd_size);

		if (c <= 0)
			break;
		rt_debugk("%s: xmit %d of %d chars\n", 
			  p->name, c, p->obuf.avbs);

		bdp = p->tx_cur;
		if (bdp->cbd_sc & BD_SC_READY) {
			//p->flags |= TX_WAKEUP;
			break;
		}

		/* Get next BD.
		 */
		if (bdp->cbd_sc & BD_SC_WRAP)
			p->tx_cur = p->tx_bd_base;
		else
			p->tx_cur = (cbd_t *)bdp + 1;

		cp = p->tx_va_base + ((bdp - p->tx_bd_base) * p->tx_bd_size);

		mbxget(&p->obuf, cp, c);

		bdp->cbd_datlen = c;
		bdp->cbd_sc |= BD_SC_READY;

		ret += c;
	}
	return ret;
}

/* 
 * We use a serive task to handle interrupts and transmit requests
 * in a serialized way.
 */
static void sp8xx_service_task(int data)
{
	rt_spct_t *p = (rt_spct_t *)data;
	u_char	events;
	volatile smc_t	*smcp;
	volatile scc_t	*sccp;
	int num, is_scc, rxed, txed, nrx, ntx;

	rt_printk("Starting service task for %s\n", p->name); 

	num = PORT_NUM(p->state->smc_scc_num);
	is_scc = PORT_IS_SCC(p->state->smc_scc_num);

	rt_enable_irq(p->state->irq);

	while (1) {
		if (rt_sem_wait(&p->service_sem) >= 0xffff)
			break;
		/* We got something to do */
		ntx = nrx = 0;
		if (is_scc) {
			sccp = &cpmp->cp_scc[num];
			events = sccp->scc_scce;
			if (events & SMCM_BRKE) {
				p->error |= RT_SP_BREAK;
				//receive_break(p);
			}
			if (events & SCCM_RX)
				nrx = sp8xx_receive_chars(p);
			// if (events & SCCM_TX)
			ntx = sp8xx_transmit_chars(p);
			if (events)
				sccp->scc_scce = events;
		}
		else {
			smcp = &cpmp->cp_smc[num];
			events = smcp->smc_smce;
			if (events & SMCM_BRKE) {
				p->error |= RT_SP_BREAK;
				//receive_break(p);
			}
			if (events & SMCM_RX)
				nrx = sp8xx_receive_chars(p);
			// if (events & SMCM_TX)
			ntx = sp8xx_transmit_chars(p);
			if (events)
				smcp->smc_smce = events;
		}
		rt_debugk("%s: events=0x%x ntx=%d nrx=%d\n", p->name, events, ntx, nrx);

		/* Call the error callback function if it is defined */
		p->call_usr = 0;
		if (p->error) {
			if (p->err_callback_fun) {
				(p->err_callback_fun)(p->error);
			} else if (p->callback_task) {
				p->call_usr = 1;
			}                                                               
		}

		/*
		 * Call the callback function if it is defined and chars in receive 
		 * buffer are more than rxthrs or free chars in transmit buffer are 
		 * more than txthrs.
		 */
		rxed = nrx && p->rxthrs && p->ibuf.avbs >= abs(p->rxthrs) ? p->rxthrs : 0;
		txed = ntx && p->txthrs && p->obuf.frbs >= abs(p->txthrs) ? p->txthrs : 0;
		if (rxed < 0 && txed < 0 && p->rxsem.count < 0 && p->txsem.count < 0) {
			p->rxthrs = p->txthrs = 0;
			rt_unmask_irq(p->state->irq);
			if (p->rxsem.queue.next->task->priority < 
			    p->txsem.queue.next->task->priority) {
				rt_sem_signal(&p->rxsem);
				rt_sem_signal(&p->txsem);
			} else {
				rt_sem_signal(&p->txsem);
				rt_sem_signal(&p->rxsem);
			}
			continue;
		}
		if (rxed < 0 && p->rxsem.count < 0) {
			p->rxthrs = 0;
			rt_unmask_irq(p->state->irq);
			rt_sem_signal(&p->rxsem);
			continue;
		}
		if (txed < 0 && p->txsem.count < 0) {
			p->txthrs = 0;
			rt_unmask_irq(p->state->irq);
			rt_sem_signal(&p->txsem);
			continue;
		}
		if (rxed || txed) {
			if (p->callback_fun) {
				(p->callback_fun)(p->ibuf.avbs, p->obuf.frbs);
				rt_unmask_irq(p->state->irq);
				continue;
			} else if (p->callback_task) {
				p->call_usr |= 2;
			}
		}
		if (p->call_usr) {
			rt_unmask_irq(p->state->irq);
			rt_task_resume(p->callback_task);
			continue;
		} 
		rt_unmask_irq(p->state->irq);
	}
	rt_printk("Service task for %s terminated.\n", p->name);
}
/* Set a baud rate generator.  This needs lots of work.  There are
 * four BRGs, any of which can be wired to any channel.
 * The internal baud rate clock is the system clock divided by 16.
 * This assumes the baudrate is 16x oversampled by the uart.
 */
#define BRG_INT_CLK             (((bd_t *)__res)->bi_intfreq)
#define BRG_UART_CLK            (BRG_INT_CLK/16)
#define BRG_UART_CLK_DIV16      (BRG_UART_CLK/16)

void sp8xx_cpm_setbrg(uint brg, uint rate)
{
	volatile uint   *bp;

	/* This is good enough to get SMCs running.....
	 */
	bp = (uint *)&cpmp->cp_brgc1;
	bp += brg;
	/* The BRG has a 12-bit counter.  For really slow baud rates (or
	 * really fast processors), we may have to further divide by 16.
	 */
	if (((BRG_UART_CLK / rate) - 1) < 4096)
		*bp = (((BRG_UART_CLK / rate) - 1) << 1) | CPM_BRG_EN;
	else
		*bp = (((BRG_UART_CLK_DIV16 / rate) - 1) << 1) |
			CPM_BRG_EN | CPM_BRG_DIV16;
}

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void sp8xx_change_speed(rt_spct_t *p, int baud_rate, 
				int bits, int stopbits, int parity, int mode)
{
	unsigned cval, scval, prev_mode, new_mode;
	int	sbits, num;
	//unsigned long	flags;
	struct rt_spstate *state = p->state;
	volatile smc_t	*smcp;
	volatile scc_t	*sccp;


	num = PORT_NUM(state->smc_scc_num);

	/* Character length programmed into the mode register is the
	 * sum of: 1 start bit, number of data bits, 0 or 1 parity bit,
	 * 1 or 2 stop bits, minus 1.
	 * The value 'bits' counts this for us.
	 */
	cval = 0;
	scval = 0;

#ifdef CPM_UART_HANDSHAKING
	if (PORT_IS_SCC(state->smc_scc_num)) {
#ifdef RTAI_UART_CTS_CONTROL_SCC1
		if ( num == 0 )
			scval = SCU_PMSR_FLC;
#endif
#ifdef RTAI_UART_CTS_CONTROL_SCC2
		if ( num == 1 )
			scval = SCU_PMSR_FLC;
#endif
#ifdef RTAI_UART_CTS_CONTROL_SCC3
		if ( num == 2 )
			scval = SCU_PMSR_FLC;
#endif
#ifdef RTAI_UART_CTS_CONTROL_SCC4
		if ( num == 3 )
			scval = SCU_PMSR_FLC;
#endif
	}
#endif

	/* byte size and parity */
	sbits = bits - 5;

	if (stopbits > 1) {
		cval |= SMCMR_SL;	/* Two stops */
		scval |= SCU_PMSR_SL;
		bits++;
	}
	if (parity != RT_SP_PARITY_NONE) {
		cval |= SMCMR_PEN;
		scval |= SCU_PMSR_PEN;
		bits++;
		if (parity & RT_SP_PARITY_EVEN) {
			cval |= SMCMR_PM_EVEN;
			scval |= (SCU_PMSR_REVP | SCU_PMSR_TEVP);
		}
	}

	/*
	 * Set up parity check flag
	 */
#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

#ifdef IS_READY
	p->read_status_mask = (BD_SC_EMPTY | BD_SC_OV);

	if (I_INPCK(p->tty))
		p->read_status_mask |= BD_SC_FR | BD_SC_PR;
	if (I_BRKINT(p->tty) || I_PARMRK(p->tty))
		p->read_status_mask |= BD_SC_BR;

	/*
	 * Characters to ignore
	 */
	p->ignore_status_mask = 0;
	if (I_IGNPAR(p->tty))
		p->ignore_status_mask |= BD_SC_PR | BD_SC_FR;
	if (I_IGNBRK(p->tty)) {
		p->ignore_status_mask |= BD_SC_BR;
		/*
		 * If we're ignore parity and break indicators, ignore
		 * overruns too.  (For real raw support).
		 */
		if (I_IGNPAR(p->tty))
			p->ignore_status_mask |= BD_SC_OV;
	}
	/*
	 * !!! ignore all characters if CREAD is not set
	 */
	if ((cflag & CREAD) == 0)
		p->read_status_mask &= ~BD_SC_EMPTY;
#endif

	/* Start bit has not been added (so don't, because we would just
	 * subtract it later), and we need to add one for the number of
	 * stops bits (there is always at least one).
	 */
	bits++;
	if (PORT_IS_SCC(state->smc_scc_num)) {
		sccp = &cpmp->cp_scc[num];
		new_mode = (sbits << 12) | scval;
		prev_mode = sccp->scc_pmsr;
		if (!(prev_mode & SCU_PMSR_PEN))
			/* If parity is disabled, mask out even/odd */
			prev_mode &= ~(SCU_PMSR_TPM|SCU_PMSR_RPM);
		if (prev_mode != new_mode)
			sccp->scc_pmsr = new_mode;
	}
	else {
		smcp = &cpmp->cp_smc[num];

		/* Set the mode register.  We want to keep a copy of the
		 * enables, because we want to put them back if they were
		 * present.
		 */
		prev_mode = smcp->smc_smcmr & (SMCMR_REN | SMCMR_TEN);
		new_mode = smcr_mk_clen(bits) | cval | SMCMR_SM_UART
			| prev_mode;
		if (!(prev_mode & SMCMR_PEN))
			/* If parity is disabled, mask out even/odd */
			prev_mode &= ~SMCMR_PM_EVEN;
		debugk("new_mode=0x%x\n", new_mode);
		if (prev_mode != new_mode)
			smcp->smc_smcmr = new_mode;
	}

	sp8xx_cpm_setbrg(PORT_BRG(state->smc_scc_num), baud_rate);
}

/*
 * rt_spopen
 *
 * Open the serial port
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * 		baud		50 .. 115200
 *
 *      numbits		5,6,7,8
 *      
 *      stopbits	1,2
 *					
 *      
 *      parity		RT_SP_PARITY_NONE
 *      		RT_SP_PARITY_EVEN
 *      		RT_SP_PARITY_ODD
 *      		RT_SP_PARITY_HIGH
 *      		RT_SP_PARITY_LOW
 *      			
 * 	mode		RT_SP_NO_HAND_SHAKE
 * 			RT_SP_DSR_ON_TX
 * 			RT_SP_HW_FLOW
 *
 *      fifotrig	RT_SP_FIFO_DISABLE
 *      		RT_SP_FIFO_SIZE_1
 *      		RT_SP_FIFO_SIZE_4
 *      		RT_SP_FIFO_SIZE_8
 *      		RT_SP_FIFO_SIZE_14
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EINVAL		if wrong parameter value
 * 		-EADDRINUSE	if trying to open an openend port
 *
 */ 
int rt_spopen(unsigned int tty, unsigned int baud, unsigned int numbits,
              unsigned int stopbits, unsigned int parity, int mode,
	      int fifotrig)
{
	rt_spct_t *p;

	unsigned long flags;
#ifdef IS_READY
	int	retval=0;
#endif
	int	num;
	struct rt_spstate *state;
	volatile smc_t		*smcp;
	volatile scc_t		*sccp;
	volatile smc_uart_t	*up;
	volatile scc_uart_t	*scup;
#ifdef CPM_UART_HANDSHAKING
	volatile immap_t	*immap;
#endif

	debugk("rt_spopen(%d,%d,%d,%d,%d,%d,%d)\n", tty, baud, numbits,
	       stopbits, parity, mode, fifotrig);

	CHECK_SPINDX(tty);
	if ( baud<50 || baud > 115200 ||
	     (mode&0x03) != mode ||
	     (parity&0x38) != parity ||
		 stopbits<1 || stopbits>2 ||
		 numbits<5 || numbits>8 ||
	     (fifotrig&0xC0) != fifotrig )
		return -EINVAL;	

	p = &spct[tty];
	if (p->opened) {
		return -EADDRINUSE;
		
	}
	MOD_INC_USE_COUNT;

	state = p->state;
	
	/* Disable interrupt source early */ 
	rt_disable_irq(state->irq);

	num = PORT_NUM(state->smc_scc_num);

	flags = rt_spin_lock_irqsave(&p->lock);

#ifdef CPM_UART_HANDSHAKING
	immap = (immap_t *)IMAP_ADDR;
	if (PORT_IS_SCC(state->smc_scc_num)) {
	  //if (p->tty->termios->c_cflag & CBAUD) {
#ifdef RTAI_UART_DTR_CONTROL_SCC1
			if ( num == 0 )
				PORT_DTR1_DAT &= ~DTR1_PIN;
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC2
			if ( num == 1 )
				PORT_DTR2_DAT &= ~DTR2_PIN;
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC3
			if ( num == 2 )
				PORT_DTR3_DAT &= ~DTR3_PIN;
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC4
			if ( num == 3 )
				PORT_DTR4_DAT &= ~DTR4_PIN;
#endif /* CPM_UART_HANDSHAKING */
			//}
        }
#endif

	/*
	 * Set the speed of the serial port
	 */
	sp8xx_change_speed(p, baud, numbits, stopbits, parity, mode);

	if (PORT_IS_SCC(p->state->smc_scc_num)) {
		sccp = &cpmp->cp_scc[num];
		scup = (scc_uart_t *)&cpmp->cp_dparam[state->port];
		scup->scc_genscc.scc_mrblr = p->rx_bd_size;

		scup->scc_maxidl = p->rx_bd_size;

#ifdef RTAI_UART_MAXIDL_SCC1
		if ( num == 0 )
			scup->scc_maxidl = RTAI_UART_MAXIDL_SCC1;
#endif
#ifdef RTAI_UART_MAXIDL_SCC2
		if ( num == 1 )
			scup->scc_maxidl = RTAI_UART_MAXIDL_SCC2;
#endif
#ifdef RTAI_UART_MAXIDL_SCC3
		if ( num == 2 )
			scup->scc_maxidl = RTAI_UART_MAXIDL_SCC3;
#endif
#ifdef RTAI_UART_MAXIDL_SCC4
		if ( num == 3 )
			scup->scc_maxidl = RTAI_UART_MAXIDL_SCC4;
#endif

		sccp->scc_sccm |= (UART_SCCM_TX | UART_SCCM_RX);
		sccp->scc_gsmrl |= (SCC_GSMRL_ENR | SCC_GSMRL_ENT);
	}
	else {
		smcp = &cpmp->cp_smc[num];

		/* Enable interrupts and I/O.
		*/
		smcp->smc_smcm |= (SMCM_RX | SMCM_TX);
		smcp->smc_smcmr |= (SMCMR_REN | SMCMR_TEN);

		/* We can tune the buffer length and idle characters
		 * to take advantage of the entire incoming buffer size.
		 * If mrblr is something other than 1, maxidl has to be
		 * non-zero or we never get an interrupt.  The maxidl
		 * is the number of character times we wait after reception
		 * of the last character before we decide no more characters
		 * are coming.
		 */
		up = (smc_uart_t *)&cpmp->cp_dparam[state->port];
		up->smc_mrblr = p->rx_bd_size;

		up->smc_maxidl = p->rx_bd_size;

#ifdef RTAI_UART_MAXIDL_SMC2
		if ( num == 1 )
			up->smc_maxidl = RTAI_UART_MAXIDL_SMC2;
#endif
		up->smc_brkcr = 1;	/* number of break chars */

		debugk("num=%d port=0x%lx\n", num, state->port);
		debugk("smcp at 0x%p, up at 0x%p\n", smcp, up);
		debugk("smc_smcm  = 0x%x\n", smcp->smc_smcm);
		debugk("smc_smcmr = 0x%x\n", smcp->smc_smcmr);
		debugk("smc_smce  = 0x%x\n", smcp->smc_smce);
		debugk("smc_mrblr = 0x%x\n", up->smc_mrblr);
		debugk("smc_maxidl= 0x%x\n", up->smc_maxidl);
		debugk("smc_brkcr = 0x%x\n", up->smc_brkcr);
	}

	rt_spin_unlock_irqrestore(flags, &p->lock);

	/* Reset error */
	p->error = 0;

	/* Initialize received and transmit buffers */
	mbx_init(&p->ibuf);
	mbx_init(&p->obuf);
	p->just_oner = p->just_onew = 0;
	/* Remember that it is opened */
	p->callback_task = 0;

	/* Initialize binary semaphore and start RT service task */
        rt_typed_sem_init(&p->service_sem, 0, BIN_SEM);
	rt_task_init(&p->service_thread, sp8xx_service_task, (int)p, 
		     STACK_SIZE, spprio[tty], 0, 0);
	rt_task_resume(&p->service_thread);

	p->opened = 1;

	return 0;
}


/*
 * rt_spclose
 *
 * Close the serial port
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 *
 */ 
int rt_spclose(unsigned int tty)
{
	rt_spct_t *p;
	unsigned long	flags;
	struct rt_spstate *state;
	int		num;
	volatile smc_t	*smcp;
	volatile scc_t	*sccp;
#ifdef CPM_UART_HANDSHAKING
	volatile immap_t	*immap;
#endif

	if (!(p = get_spct(tty)))
		return -ENODEV;
	debugk("Shutting down %s (irq %d)....", name, state->irq);

	state = p->state;

	/* Disable interrupt generation for UART */
	rt_disable_irq(state->irq);

	flags = rt_spin_lock_irqsave(&p->lock);

	num = PORT_NUM(state->smc_scc_num);

	if (PORT_IS_SCC(state->smc_scc_num)) {
		sccp = &cpmp->cp_scc[num];
		sccp->scc_sccm &= ~(UART_SCCM_TX | UART_SCCM_RX);
		sccp->scc_gsmrl &= ~(SCC_GSMRL_ENR | SCC_GSMRL_ENT);
	}
	else {
		smcp = &cpmp->cp_smc[num];

		/* Disable interrupts and I/O.
		 */
		debugk("smc_smcm=0x%x smc_smcmr=0x%x smc_smce=0x%x\n",
		       smcp->smc_smcm, smcp->smc_smcmr, smcp->smc_smce);
		smcp->smc_smcm &= ~(SMCM_RX | SMCM_TX);
		smcp->smc_smcmr &= ~(SMCMR_REN | SMCMR_TEN);
	}
#ifdef CPM_UART_HANDSHAKING
	immap = (immap_t *)IMAP_ADDR;
	if (PORT_IS_SCC(state->smc_scc_num)) {
	  //if (!info->tty || (info->tty->termios->c_cflag & HUPCL)) {
#ifdef RTAI_UART_DTR_CONTROL_SCC1
			if ( num == 0 )
				PORT_DTR1_DAT |= DTR1_PIN;
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC2
			if ( num == 1 )
				PORT_DTR2_DAT |= DTR2_PIN;
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC3
			if ( num == 2 )
				PORT_DTR3_DAT |= DTR3_PIN;
#endif
#ifdef RTAI_UART_DTR_CONTROL_SCC4
			if ( num == 3 )
				PORT_DTR4_DAT |= DTR4_PIN;
#endif
			//}
	}
#endif

	rt_spin_unlock_irqrestore(flags, &p->lock);

	if (p->callback_task) {
		rt_task_resume(p->callback_task);
		p->callback_task = 0;
	}

	rt_sem_delete(&p->service_sem);
	rt_task_delete(&p->service_thread);
	
	MOD_DEC_USE_COUNT;
	p->opened = 0;

	return 0;
}


/*
 * rt_spset_thrs
 *
 * Open the serial port
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * 		rxthrs		number of chars in receive buffer trhreshold
 *
 * 		txthrs		number of free chars in transmit buffer threshold
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EINVAL		if wrong parameter value
 *
 */ 
int rt_spset_thrs(unsigned int tty, int rxthrs, int txthrs)
{
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (rxthrs < 0 || txthrs < 0) {
		return -EINVAL;
	}
	p->rxthrs = rxthrs;
	p->txthrs = txthrs;
	return 0;
}

/*
 * rt_spset_callback_fun
 *
 * Define the callback function to be called when the chars in the receive
 * buffer or the free chars in the transmit buffer have reached the 
 * specified thresholds
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * 		callback_fun	pointer to the callback function
 * 				the two int parameters passed to this funcion
 * 				are the number 
 *
 * 		rxthrs		number of chars in receive buffer trhreshold
 *
 * 		txthrs		number of free chars in transmit buffer threshold
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EINVAL		if wrong parameter value
 *
 */ 
int rt_spset_callback_fun(unsigned int tty, void (*callback_fun)(int, int), 
                          int rxthrs, int txthrs)
{
	int prev_callback_fun;
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (rt_spset_thrs(tty, rxthrs, txthrs)) {
		return -EINVAL;
	}
	prev_callback_fun = (int)p->callback_fun;
	p->callback_fun = callback_fun;
	return prev_callback_fun;
}


/*
 * rt_spset_err_callback_fun
 *
 * Define the callback function to be called when the interrupt 
 * service routine detect an error
 *
 * Arguments: 
 * 		tty		serial port number
 *
 * 		err_callback_fun 	pointer to the callback function
 * 					the int parameter passed to this funcion
 * 					will contain the error code 
 *
 * Return Value:
 * 		0		if success
 * 		-ENODEV		if wrong tty number
 * 		-EINVAL		if wrong parameter value
 *
 */ 
int rt_spset_err_callback_fun(unsigned int tty, void (*err_callback_fun)(int))
{
	int prev_err_callback_fun;
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	prev_err_callback_fun = (int)p->err_callback_fun;
	p->err_callback_fun = err_callback_fun;
	return prev_err_callback_fun;
}


/*
 * rt_spset_callback_fun_usr
 *
 * Define the callback function to be called from user space
 *
 */ 
int rt_spset_callback_fun_usr(unsigned int tty, unsigned long callback_fun_usr, int rxthrs, int txthrs, int code, void *task)
{
	int prev_callback_fun_usr;
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (!p->opened) {
		return -ENODEV;
	}
	if (code && !task) {
		return !p->callback_task ? EINVAL : -EINVAL;
	}
	if ((!code && !task) || rt_spset_thrs(tty, rxthrs, txthrs)) {
		return -EINVAL;
	}
	if (task) {
		p->callback_task = task;
	}
	prev_callback_fun_usr = (int)p->callback_fun_usr;
	p->callback_fun_usr = callback_fun_usr;
	return prev_callback_fun_usr;
}


/*
 * rt_spset_err_callback_fun_usr
 *
 * Define the err callback function to be called from user space
 *
 */ 
int rt_spset_err_callback_fun_usr(unsigned int tty, unsigned long err_callback_fun_usr, 
				  int dummy1, int dummy2, int code, void *task)
{
	int prev_err_callback_fun_usr;
	rt_spct_t *p;

	if (!(p = get_spct(tty)))
		return -ENODEV;
	if (code && !task) {
		return !p->callback_task ? EINVAL : -EINVAL;
	}
	if (!code && !task) {
		return -EINVAL;
	}
	if (task) {
		p->callback_task = task;
	}
	prev_err_callback_fun_usr = (int)p->err_callback_fun_usr;
	p->callback_fun_usr = err_callback_fun_usr;
	return prev_err_callback_fun_usr;
}


/*
 * rt_spwait_usr_callback
 *
 * Wait for user space callback
 *
 */ 
void rt_spwait_usr_callback(unsigned int tty, unsigned long *retvals)
{
	rt_spct_t *p;
	if (!(p = get_spct(tty)))
		return;
	
	rt_task_suspend(p->callback_task);
	retvals[0] = p->call_usr | 2 ? p->callback_fun_usr : 0;
	retvals[1] = p->call_usr | 1 ? p->err_callback_fun_usr : 0;
	retvals[2] = p->ibuf.avbs;
	retvals[3] = p->obuf.frbs;
	retvals[4] = p->error;
	retvals[5] = p->opened;
	return;
}


/* Sending a break is a two step process on the SMC/SCC.  It is accomplished
 * by sending a STOP TRANSMIT command followed by a RESTART TRANSMIT
 * command.  We take advantage of the begin/end functions to make this
 * happen.
 */
static ushort	smc_chan_map[] = {
	CPM_CR_CH_SMC1,
	CPM_CR_CH_SMC2
};

static ushort	scc_chan_map[] = {
	CPM_CR_CH_SCC1,
	CPM_CR_CH_SCC2,
	CPM_CR_CH_SCC3,
	CPM_CR_CH_SCC4
};

/*
 * The serial driver boot-time initialization code!
 */

static int sp8xx_alloc_brg(int port)
{
    static int brg = 0;
    volatile cpm8xx_t *cp = cpmp;
    int res;

    if (!brg) {
#ifdef CONFIG_USB_MPC8xx
	    /* if USB is turned on, we need to use BRG1 for SOF generation */
	    brg++;
#endif
#ifdef CONFIG_SMC1_UART
	    brg++;
#endif
#ifdef CONFIG_SMC2_UART
	    brg++;
#endif
#ifdef CONFIG_SCC1_UART
	    brg++;
#endif
#ifdef CONFIG_SCC2_UART
	    brg++;
#endif
#ifdef CONFIG_SCC3_UART
	    brg++;
#endif
#ifdef CONFIG_SCC4_UART
	    brg++;
#endif
    }
    res = brg;

    /* "Wire" the BRG to the specified port
    */
    switch (port) {
    case QUICC_CPM_SMC1:
        cp->cp_simode = (cp->cp_simode & ~(0x07<<12)) | (brg<<12);
        break;
    case QUICC_CPM_SMC2:
        cp->cp_simode = (cp->cp_simode & ~(0x07<<28)) | (brg<<28);
        break;
    case QUICC_CPM_SCC1:
        cp->cp_sicr = (cp->cp_sicr & ~(0xFF<<0)) | (((brg<<3)|(brg<<0))<<0);
        break;
    case QUICC_CPM_SCC2:
        cp->cp_sicr = (cp->cp_sicr & ~(0xFF<<8)) | (((brg<<3)|(brg<<0))<<8);
        break;
    case QUICC_CPM_SCC3:
        cp->cp_sicr = (cp->cp_sicr & ~(0xFF<<16)) | (((brg<<3)|(brg<<0))<<16);
        break;
    case QUICC_CPM_SCC4:
        cp->cp_sicr = (cp->cp_sicr & ~(0xFF<<24)) | (((brg<<3)|(brg<<0))<<24);
        break;
    }
    /* Consume this BRG - Note: the last BRG will be reused if this
    */
    /* function is called too many times!
    */
    if (brg < QUICC_MAX_BRG) brg++;
    return res;
}

int init_module(void)
{
	struct rt_spstate *state;
	rt_spct_t *p;
	uint		mem_addr, dp_addr, iobits;
	int		i, j, num, idx;
	ushort		chan;
	volatile	cbd_t		*bdp;
	volatile	cpm8xx_t	*cp;
	volatile	smc_t		*sp;
	volatile	smc_uart_t	*up;
	volatile	scc_t		*scp;
	volatile	scc_uart_t	*sup;
	volatile	immap_t		*immap;

	cp = cpmp;	/* Get pointer to Communication Processor */
	immap = (immap_t *)IMAP_ADDR;	/* and to internal registers */


	/* Configure SCC2, SCC3, and SCC4 instead of port A parallel I/O.
	 */
	iobits = 0
#if defined(RTAI_SCC1_UART)
		| 0x0003
#endif
#if defined(RTAI_SCC2_UART)
		| 0x000C
#endif
#if defined(RTAI_SCC3_UART)	&& \
   !defined(CONFIG_MBX)		&& \
   !defined(CONFIG_IP860)	&& \
   !defined(CONFIG_TQM850L)	&& \
   !defined(CONFIG_TQM850M)	&& \
   !defined(CONFIG_SM850)
		| 0x0030
#endif
#if defined(RTAI_SCC4_UART) && !defined(CONFIG_IP860)
		| 0x00C0
#endif
		;

	/* The "standard" configuration through the 860.
	 */
	immap->im_ioport.iop_papar |=  iobits;
	immap->im_ioport.iop_padir &= ~iobits;
	immap->im_ioport.iop_paodr &= ~iobits;

#if defined(RTAI_SCC3_UART) && \
   (defined(CONFIG_TQM850L) || defined(CONFIG_TQM850M) || defined(CONFIG_SM850))
	/* The MPC850 has SCC3 on Port B */
	cp->cp_pbpar |=  0x06;
	cp->cp_pbdir &= ~0x06;
	cp->cp_pbodr &= ~0x06;
#endif

#if defined(CONFIG_MBX) || defined(CONFIG_IP860)
	/*
	 * On the MBX,   SCC3 is through Port D.
	 * On the IP860, SCC3 and SCC4 are through Port D.
	 */
	iobits = 0
# if defined(RTAI_SCC3_UART)
		| 0x0030
# endif
# if defined(RTAI_SCC4_UART)
		| 0x00C0
# endif
		;
	immap->im_ioport.iop_pdpar |= iobits;
#endif	/* CONFIG_MBX, CONFIG_IP860 */

	/* Since we don't yet do modem control, connect the port C pins
	 * as general purpose I/O.  This will assert CTS and CD for the
	 * SCC ports.
	 */
	iobits = 0
#if defined(RTAI_SCC1_UART)
		| 0x0031	/* CD1, CTS1, RTS1 */
#endif
#if defined(RTAI_SCC2_UART)
		| 0x00C2	/* CD2, CTS2, RTS2 */
#endif
#if defined(RTAI_SCC3_UART)
		| 0x0304	/* CD3, CTS3, RTS3 */
#endif
#if defined(RTAI_SCC4_UART)
		| 0x0C08	/* CD4, CTS4, RTS4 */
#endif
		;

	immap->im_ioport.iop_pcdir |=  iobits;
	immap->im_ioport.iop_pcpar &= ~iobits;

	/* Here we connect handshaking signals CTS, RTS and CD to 
	 * the SCC to be controlled by CPM, and DTR as
	 * a general purpose output to be controlled by software.
	 */
#if defined(CPM_UART_HANDSHAKING)
#if (RTAI_UART_RTS_CONTROL_SCC1 == PORT_B)
	immap->im_cpm.cp_pbpar |= (1 << (31 - RTAI_RTS1_PIN));
	immap->im_cpm.cp_pbdir |= (1 << (31 - RTAI_RTS1_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC2 == PORT_B)
	immap->im_cpm.cp_pbpar |= (1 << (31 - RTAI_RTS2_PIN));
	immap->im_cpm.cp_pbdir |= (1 << (31 - RTAI_RTS2_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC3 == PORT_B)
	immap->im_cpm.cp_pbpar |= (1 << (31 - RTAI_RTS3_PIN));
	immap->im_cpm.cp_pbdir |= (1 << (31 - RTAI_RTS3_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC4 == PORT_B)
	immap->im_cpm.cp_pbpar |= (1 << (31 - RTAI_RTS4_PIN));
	immap->im_cpm.cp_pbdir |= (1 << (31 - RTAI_RTS4_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC1 == PORT_C)
	immap->im_ioport.iop_pcpar |=  (1 << (15 - RTAI_RTS1_PIN));
	immap->im_ioport.iop_pcdir &= ~(1 << (15 - RTAI_RTS1_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC2 == PORT_C)
	immap->im_ioport.iop_pcpar |=  (1 << (15 - RTAI_RTS2_PIN));
	immap->im_ioport.iop_pcdir &= ~(1 << (15 - RTAI_RTS2_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC3 == PORT_C)
	immap->im_ioport.iop_pcpar |=  (1 << (15 - RTAI_RTS3_PIN));
	immap->im_ioport.iop_pcdir &= ~(1 << (15 - RTAI_RTS3_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC4 == PORT_C)
	immap->im_ioport.iop_pcpar |=  (1 << (15 - RTAI_RTS4_PIN));
	immap->im_ioport.iop_pcdir &= ~(1 << (15 - RTAI_RTS4_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC3 == PORT_D)
	immap->im_ioport.iop_pdpar |=  (1 << (15 - RTAI_RTS3_PIN));
#endif
#if (RTAI_UART_RTS_CONTROL_SCC4 == PORT_D)
	immap->im_ioport.iop_pdpar |=  (1 << (15 - RTAI_RTS4_PIN));
#endif
        iobits = 0
#if defined(RTAI_UART_CTS_CONTROL_SCC1)
		| 0x0010
#endif
#if defined(RTAI_UART_CD_CONTROL_SCC1)
		| 0x0020
#endif
#if defined(RTAI_UART_CTS_CONTROL_SCC2)
		| 0x0040
#endif
#if defined(RTAI_UART_CD_CONTROL_SCC2)
		| 0x0080
#endif
#if defined(RTAI_UART_CTS_CONTROL_SCC3)
		| (1 << (15 - RTAI_CTS3_PIN))
#endif
#if defined(RTAI_UART_CD_CONTROL_SCC3)
		| (1 << (15 - RTAI_CD3_PIN))
#endif
#if defined(RTAI_UART_CTS_CONTROL_SCC4)
		| 0x0400
#endif
#if defined(RTAI_UART_CD_CONTROL_SCC4)
		| 0x0800
#endif
		;
	immap->im_ioport.iop_pcpar &= ~iobits;
	immap->im_ioport.iop_pcso |=  iobits;
	immap->im_ioport.iop_pcdir &= ~iobits;

#if defined(RTAI_UART_DTR_CONTROL_SCC1)
	PORT_DTR1_PAR &= ~DTR1_PIN;	/* DTR1 */
	PORT_DTR1_DAT |=  DTR1_PIN;
	PORT_DTR1_DIR |=  DTR1_PIN;
#endif
#if defined(RTAI_UART_DTR_CONTROL_SCC2)
	PORT_DTR2_PAR &= ~DTR2_PIN;	/* DTR2 */
	PORT_DTR2_DAT |=  DTR2_PIN;
	PORT_DTR2_DIR |=  DTR2_PIN;
#endif
#if defined(RTAI_UART_DTR_CONTROL_SCC3)
	PORT_DTR3_PAR &= ~DTR3_PIN;	/* DTR3 */
	PORT_DTR3_DAT |=  DTR3_PIN;
	PORT_DTR3_DIR |=  DTR3_PIN;
#endif
#if defined(RTAI_UART_DTR_CONTROL_SCC4)
	PORT_DTR4_PAR &= ~DTR4_PIN;	/* DTR4 */
	PORT_DTR4_DAT |=  DTR4_PIN;
	PORT_DTR4_DIR |=  DTR4_PIN;
#endif

#endif	/* CPM_UART_HANDSHAKING */

#ifdef CONFIG_PP04
	/* Frequentis PP04 forced to RS-232 until we know better.
	 * Port C 12 and 13 low enables RS-232 on SCC3 and SCC4.
	 */
	immap->im_ioport.iop_pcdir |= 0x000c;
	immap->im_ioport.iop_pcpar &= ~0x000c;
	immap->im_ioport.iop_pcdat &= ~0x000c;

	/* This enables the TX driver.
	*/
	cp->cp_pbpar &= ~0x6000;
	cp->cp_pbdat &= ~0x6000;
#endif	/* CONFIG_PP04 */

	spct = p = kmalloc(NR_PORTS * sizeof(rt_spct_t), GFP_KERNEL);
	if (!spct) {
		debugk(KERN_ERR "Counldn't allocate control table\n");
		return -ENOMEM;
	}
	memset(p, 0, NR_PORTS * sizeof(rt_spct_t));

	for (i = 0, state = rs_table; i < NR_PORTS; i++, state++, p++) {

		state->line = i;
		sprintf(p->name, "rtSP%d", state->line);
		PORT_BRG_SET(state->smc_scc_num, 
			     sp8xx_alloc_brg(state->smc_scc_num));
		printk(KERN_INFO "%s at 0x%04x is on %s%d using BRG%d\n",
		       p->name, (unsigned int)(state->port),
		       PORT_IS_SCC(state->smc_scc_num) ? "SCC" : "SMC",
                       PORT_NUM(state->smc_scc_num)+1,
                       PORT_BRG(state->smc_scc_num)+1);
		debugk(KERN_INFO "state->smc_scc_num=0x%x\n", state->smc_scc_num);

		p->state = state;

		/* We need to allocate a transmit and receive buffer
		 * descriptors from dual port ram, and a character
		 * buffer area from host mem.
		 */

		num = PORT_NUM(state->smc_scc_num);
		idx = PORT_IS_SCC(state->smc_scc_num) ? num : num + 4;
		p->rx_bd_size = rx_bd_sizes[idx];
		p->tx_bd_size = tx_bd_sizes[idx];

		dp_addr = m8xx_cpm_dpalloc(sizeof(cbd_t) * rx_bd_nums[idx]);
		
		if (dp_addr == CPM_DP_NOSPACE)
			panic( "uart.c: could not allocate BD" );
		

		/* Allocate space for FIFOs in the host memory.
		 */
		mem_addr = m8xx_cpm_hostalloc(rx_bd_nums[idx] * p->rx_bd_size);
		
		if (!mem_addr)
			panic( "uart.c: could not allocate serial buffer" );
		
		p->rx_dp_addr = (unsigned short)dp_addr;
		p->rx_va_base = (unsigned char *)mem_addr;

		/* Set the physical address of the host memory
		 * buffers in the buffer descriptors, and the
		 * virtual address for us to work with.
		 */
		bdp = (cbd_t *)&cp->cp_dpmem[dp_addr];
		p->rx_cur = p->rx_bd_base = (cbd_t *)bdp;

		for (j = 0; j < rx_bd_nums[idx] - 1; j++) {
			bdp->cbd_bufaddr = __pa(mem_addr);
			bdp->cbd_sc = BD_SC_EMPTY | BD_SC_INTRPT;
			mem_addr += p->rx_bd_size;
			bdp++;
		}
		bdp->cbd_bufaddr = iopa(mem_addr);
		bdp->cbd_sc = BD_SC_WRAP | BD_SC_EMPTY | BD_SC_INTRPT;
		
		if (PORT_IS_SCC(p->state->smc_scc_num)) {
			scp = &cp->cp_scc[num];
			sup = (scc_uart_t *)&cp->cp_dparam[state->port];
			sup->scc_genscc.scc_rbase = dp_addr;
		}
		else {
			sp = &cp->cp_smc[num];
			up = (smc_uart_t *)&cp->cp_dparam[state->port];
			up->smc_rbase = dp_addr;
		}

		dp_addr = m8xx_cpm_dpalloc(sizeof(cbd_t) * tx_bd_nums[idx]);

		if (dp_addr == CPM_DP_NOSPACE)
			panic( "uart.c: could not allocate BD" );
		
		/* Allocate space for FIFOs in the host memory.
		 */
		mem_addr = m8xx_cpm_hostalloc(tx_bd_nums[idx] * p->tx_bd_size);
		
		if (!mem_addr)
			panic( "uart.c: could not allocate serial buffer" );
		
		p->tx_dp_addr = (unsigned short)dp_addr;
		p->tx_va_base = (unsigned char *)mem_addr;
		
		/* Set the physical address of the host memory
		 * buffers in the buffer descriptors, and the
		 * virtual address for us to work with.
		 */
		bdp = (cbd_t *)&cp->cp_dpmem[dp_addr];
		p->tx_cur = p->tx_bd_base = (cbd_t *)bdp;
		
		for (j = 0; j < tx_bd_nums[idx] - 1; j++) {
			bdp->cbd_bufaddr = __pa(mem_addr);
			bdp->cbd_sc = BD_SC_INTRPT;
			mem_addr += p->tx_bd_size;
			bdp++;
		}
		bdp->cbd_bufaddr = iopa(mem_addr);
		bdp->cbd_sc = (BD_SC_WRAP | BD_SC_INTRPT);
		
		if (PORT_IS_SCC(p->state->smc_scc_num)) {
			
			/* Connect SCC to NMSI.
			 * Connect corresponding BRG, too.
			 */
			iobits = 0x000000FF << (num*8);	/* clear GRx, SCx */
			cp->cp_sicr &= ~iobits;
			iobits  = (PORT_BRG(p->state->smc_scc_num) &
				   0x03) << 3;	/* Rx BRG */
			iobits |= (PORT_BRG(p->state->smc_scc_num) &
				   0x03);	/* Tx BRG */
			iobits <<= (num*8);
			cp->cp_sicr |= iobits;
			
			sup->scc_genscc.scc_tbase = dp_addr;
			
			/* Set up the uart parameters in the
			 * parameter ram.
			 */
			sup->scc_genscc.scc_rfcr = SMC_EB;
			sup->scc_genscc.scc_tfcr = SMC_EB;

			/* Set this to 1 for now, so we get single
			 * character interrupts.  Using idle character
			 * time requires some additional tuning.
			 */
			sup->scc_genscc.scc_mrblr = 1;
			sup->scc_maxidl = 0;
			sup->scc_brkcr = 1;
			sup->scc_parec = 0;
			sup->scc_frmec = 0;
			sup->scc_nosec = 0;
			sup->scc_brkec = 0;
			sup->scc_uaddr1 = 0;
			sup->scc_uaddr2 = 0;
			sup->scc_toseq = 0;
			sup->scc_char1 = 0x8000;
			sup->scc_char2 = 0x8000;
			sup->scc_char3 = 0x8000;
			sup->scc_char4 = 0x8000;
			sup->scc_char5 = 0x8000;
			sup->scc_char6 = 0x8000;
			sup->scc_char7 = 0x8000;
			sup->scc_char8 = 0x8000;
			sup->scc_rccm = 0xc0ff;

			/* Send the CPM an initialize command.
			*/
			chan = scc_chan_map[num];

			cp->cp_cpcr = mk_cr_cmd(chan,
					CPM_CR_INIT_TRX) | CPM_CR_FLG;
			while (cp->cp_cpcr & CPM_CR_FLG);

			/* Set UART mode, 8 bit, no parity, one stop.
			 * Enable receive and transmit.
			 */
			scp->scc_gsmrh = 0;
			scp->scc_gsmrl =
				(SCC_GSMRL_MODE_UART | SCC_GSMRL_TDCR_16 | SCC_GSMRL_RDCR_16);

			/* Disable all interrupts and clear all pending
			 * events.
			 */
			scp->scc_sccm = 0;
			scp->scc_scce = 0xffff;
			scp->scc_dsr  = 0x7e7e;
			scp->scc_pmsr = 0x3000;

		}
		else {
			/* Configure SMCs Tx/Rx instead of port B
			 * parallel I/O.  On 823/850 these are on
			 * port A for SMC2.
			 */
#ifndef RTAI_ALTSMC2
			iobits = 0xc0 << (num * 4);
			cp->cp_pbpar |= iobits;
			cp->cp_pbdir &= ~iobits;
			cp->cp_pbodr &= ~iobits;
#else
			iobits = 0xc0;
			if (num == 0) {
				/* SMC1 on Port B, like all 8xx.
				 */
				cp->cp_pbpar |= iobits;
				cp->cp_pbdir &= ~iobits;
				cp->cp_pbodr &= ~iobits;
			}
			else {
				/* SMC2 is on Port A.
				 */
				immap->im_ioport.iop_papar |= iobits;
				immap->im_ioport.iop_padir &= ~iobits;
				immap->im_ioport.iop_paodr &= ~iobits;
			}
#endif /* RTAI_ALTSMC2 */
			
			/* Connect the baud rate generator to the
			 * SMC based upon index in rs_table.  Also
			 * make sure it is connected to NMSI.
			 */
			cp->cp_simode &= ~(0xffff << (num * 16));
			cp->cp_simode |=
				(PORT_BRG(state->smc_scc_num) << ((num * 16) + 12));
			
			up->smc_tbase = dp_addr;
			
			/* Set up the uart parameters in the
			 * parameter ram.
			 */
			up->smc_rfcr = SMC_EB;
			up->smc_tfcr = SMC_EB;
			
			/* Set this to 1 for now, so we get single
			 * character interrupts.  Using idle character
			 * time requires some additional tuning.
			 */
			up->smc_mrblr = 1;
			up->smc_maxidl = 0;
			up->smc_brkcr = 1;

			/* Send the CPM an initialize command.
			*/
			chan = smc_chan_map[num];

			cp->cp_cpcr = mk_cr_cmd(chan,
					CPM_CR_INIT_TRX) | CPM_CR_FLG;
			while (cp->cp_cpcr & CPM_CR_FLG);

			/* Set UART mode, 8 bit, no parity, one stop.
			 * Enable receive and transmit.
			 */
			sp->smc_smcmr = smcr_mk_clen(9) | SMCMR_SM_UART;

			/* Disable all interrupts and clear all pending
			 * events.
			 */
			sp->smc_smcm = 0;
			sp->smc_smce = 0xff;

		}

		/* Install interrupt handler.
		 */
		state->irq += CPM_IRQ_OFFSET;
		rt_disable_irq(state->irq);
		if (rt_request_global_irq_ext(state->irq, (void *)rt_spisr, 
					      (unsigned long)p)) {
			printk(KERN_ERR "Couldn't request global IRQ %d\n", state->irq);
			return -ENODEV;
		}

		p->ibuf.bufadr = kmalloc(2 * spbufsiz, GFP_KERNEL);
		if (!p->ibuf.bufadr) {
			// FIEXME: handle error
			return -ENOMEM;
		}
		p->obuf.bufadr = p->ibuf.bufadr + spbufsiz;
		
		p->callback_task = 0;
		p->opened = 0;
		// sef default thresholds for callback function 
		p->rxthrs = p->txthrs = 0;
		rt_sem_init(&p->txsem, 0);
		rt_sem_init(&p->rxsem, 0);

		spcnt++;
	}

	return 0;
}

void cleanup_module(void)
{
	rt_spct_t *p;
	int i;

	for (i = 0; i < spcnt; i++) {
		p = &spct[i];
		if (p->opened) {
			rt_spclose(i);
		}
		if (p->callback_task) {
			rt_task_resume(p->callback_task);
		}
		m8xx_cpm_dpfree((unsigned int)p->rx_dp_addr);
		m8xx_cpm_dpfree((unsigned int)p->tx_dp_addr);
		rt_free_global_irq(p->state->irq);
		kfree(p->ibuf.bufadr);
		kfree(p->obuf.bufadr);
		rt_sem_delete(&p->txsem);
		rt_sem_delete(&p->rxsem);
	}
	kfree(spct);
}

EXPORT_SYMBOL(rt_spclear_rx);
EXPORT_SYMBOL(rt_spclear_tx);
EXPORT_SYMBOL(rt_spget_err);
EXPORT_SYMBOL(rt_spget_msr);
EXPORT_SYMBOL(rt_spset_mcr);
EXPORT_SYMBOL(rt_spset_mode);
EXPORT_SYMBOL(rt_spset_fifotrig);
EXPORT_SYMBOL(rt_spopen);
EXPORT_SYMBOL(rt_spclose);
EXPORT_SYMBOL(rt_spread);
EXPORT_SYMBOL(rt_spevdrp);
EXPORT_SYMBOL(rt_spwrite);
EXPORT_SYMBOL(rt_spset_thrs);
EXPORT_SYMBOL(rt_spset_callback_fun);
EXPORT_SYMBOL(rt_spget_rxavbs);
EXPORT_SYMBOL(rt_spget_txfrbs);
EXPORT_SYMBOL(rt_spset_err_callback_fun);
EXPORT_SYMBOL(rt_spset_callback_fun_usr);
EXPORT_SYMBOL(rt_spset_err_callback_fun_usr);
EXPORT_SYMBOL(rt_spwait_usr_callback);
EXPORT_SYMBOL(rt_spread_timed);
EXPORT_SYMBOL(rt_spwrite_timed);


