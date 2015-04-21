/*
COPYRIGHT (C) 2002  Giuseppe Renoldi (giuseppe@renoldi.org)
                    Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#ifndef _RTAI_SPDRV_LXRT_H_
#define _RTAI_SPDRV_LXRT_H_

#include <rtai_spdrv.h>

#define  FUN_EXT_RTAI_SP  14

#define _SPOPEN      	  	 0 
#define _SPCLOSE    	  	 1	 
#define _SPREAD     	  	 2
#define _SPEVDRP      	  	 3
#define _SPWRITE     	  	 4
#define _SPCLEAR_RX  	  	 5
#define _SPCLEAR_TX  	  	 6
#define _SPGET_MSR   	  	 7
#define _SPSET_MCR   	  	 8
#define _SPGET_ERR  	  	 9
#define _SPSET_MODE	 	10
#define _SPSET_FIFOTRIG	 	11
#define _SPGET_RXAVBS	 	12
#define _SPGET_TXFRBS	 	13
#define _SPSET_THRS 	 	14
#define _SPSET_CALLBACK  	15
#define _SPSET_ERR_CALLBACK 	16
#define _SPWAIT_USR_CALLBACK 	17
#define _SPREAD_TIMED	 	18
#define _SPWRITE_TIMED  	19

#ifndef __KERNEL__

#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include <rtai_declare.h>
#include <rtai_lxrt.h>
				
DECLARE int rt_spopen(unsigned int tty, unsigned int baud, unsigned int numbits, unsigned int stopbits, unsigned int parity, int mode, int fifotrig)
{
	struct { unsigned int tty, baud, numbits, stopbits, parity; int mode, fifotrig; } arg = { tty, baud, numbits, stopbits, parity, mode, fifotrig }; 
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPOPEN, &arg).i[LOW];
}

DECLARE int rt_spclose(unsigned int tty)
{
	struct { unsigned int tty; } arg = { tty }; 
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPCLOSE, &arg).i[LOW];
}

DECLARE int rt_spread(unsigned int tty, char *msg, int msg_size)
{
	int notrd, size;
	char lmsg[size = abs(msg_size)];
	struct { unsigned int tty; char *msg; int msg_size; } arg = { tty, lmsg, msg_size };
	notrd = rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPREAD, &arg).i[LOW];
	if (notrd >= 0 && notrd != size) {
		memcpy(msg, lmsg, size - notrd);	
	}
	return notrd;
}

DECLARE int rt_spevdrp(unsigned int tty, char *msg, int msg_size)
{
	int notrd, size;
	char lmsg[size = abs(msg_size)];
	struct { unsigned int tty; char *msg; int msg_size; } arg = { tty, lmsg, msg_size };
	notrd = rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPEVDRP, &arg).i[LOW];
	if ( notrd >= 0 && notrd != size ) {
		memcpy(msg, lmsg, size - notrd);	
	}
	return notrd;
}

DECLARE int rt_spwrite(unsigned int tty, char *msg, int msg_size)
{
	int size;
	char lmsg[size = abs(msg_size)];
	struct { unsigned int tty; char *msg; int msg_size; } arg = { tty, lmsg, msg_size };
	memcpy(lmsg, msg, size);	
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPWRITE, &arg).i[LOW];
}

DECLARE int rt_spread_timed(unsigned int tty, char *msg, int msg_size, RTIME delay)
{
	struct { unsigned int tty; char *msg; int msg_size; RTIME delay; } arg = { tty, msg, msg_size, delay };
	return msg_size > 0 ? rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPREAD_TIMED, &arg).i[LOW] : msg_size;
}

DECLARE int rt_spwrite_timed(unsigned int tty, char *msg, int msg_size, RTIME delay)
{
	struct { unsigned int tty; char *msg; int msg_size; RTIME delay; } arg = { tty, msg, msg_size, delay };
	return msg_size > 0 ? rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPWRITE_TIMED, &arg).i[LOW] : msg_size;
}

DECLARE int rt_spclear_rx(unsigned int tty)
{
	struct { unsigned int tty; } arg = { tty };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPCLEAR_RX, &arg).i[LOW];
}

DECLARE int rt_spclear_tx(unsigned int tty)
{
	struct { unsigned int tty; } arg = { tty };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPCLEAR_TX, &arg).i[LOW];
}

DECLARE int rt_spget_msr(unsigned int tty, int mask)
{
	struct { unsigned int tty; int mask; } arg = { tty, mask };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPGET_MSR, &arg).i[LOW];
}

DECLARE int rt_spset_mcr(unsigned int tty, int mask, int setbits)
{
	struct { unsigned int tty; int mask, setbits; } arg = { tty, mask, setbits };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPSET_MCR, &arg).i[LOW];
}

DECLARE int rt_spget_err(unsigned int tty)
{
	struct { unsigned int tty; } arg = { tty };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPGET_ERR, &arg).i[LOW];
}

DECLARE int rt_spset_mode(unsigned int tty, int mode)
{
	struct { unsigned int tty; int mode; } arg = { tty, mode };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPSET_MODE, &arg).i[LOW];
}

DECLARE int rt_spset_fifotrig(unsigned int tty, int fifotrig)
{
	struct { unsigned int tty; int fifotrig; } arg = { tty, fifotrig };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPSET_FIFOTRIG, &arg).i[LOW];
}

DECLARE int rt_spget_rxavbs(unsigned int tty)
{
	struct { unsigned int tty; } arg = { tty };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPGET_RXAVBS, &arg).i[LOW];
}

DECLARE int rt_spget_txfrbs(unsigned int tty)
{
	struct { unsigned int tty; } arg = { tty };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPGET_TXFRBS, &arg).i[LOW];
}

DECLARE int rt_spset_thrs(unsigned int tty, int rxthrs, int txthrs)
{
	struct { unsigned int tty; int rxthrs, txthrs; } arg = { tty, rxthrs, txthrs };
	return rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPSET_THRS, &arg).i[LOW];
}

static inline void rt_spwait_usr_callback(unsigned int tty, unsigned long *retvals)
{
	struct { unsigned int tty; unsigned long *retvals; int size; } arg = { tty, retvals, 6*sizeof(unsigned long) };
	rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPWAIT_USR_CALLBACK, &arg);
	return;
}

#ifndef __CALLBACK_THREAD__
#define __CALLBACK_THREAD__
static void *callback_thread(void *farg)
{
	unsigned long retvals[6];
	struct farg_t { int tty; void *callback_fun; int rxthrs, txthrs, code; RT_TASK *task; } *arg;

	arg = (struct farg_t *)farg;
	if (!(arg->task = rt_task_init_schmod((unsigned long)arg, 0, 0, 0, SCHED_FIFO, 0xF))) {
		printf("CANNOT INIT USER SPACE CALLBACK SUPPORT\n");
		return (void *)1;
	}
	retvals[0] = arg->code;
	arg->code = 0;
	if (rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, retvals[0], arg).i[LOW] < 0) {
		printf("CANNOT SET USER SPACE CALLBACK SUPPORT\n");
		rt_task_delete(arg->task);
		free(arg);
		return (void *)1;
	}
	mlockall(MCL_CURRENT | MCL_FUTURE);

	rt_make_hard_real_time();
	while(1) {
		rt_spwait_usr_callback(arg->tty, retvals);
		if (!retvals[5]) break;
		if (retvals[0]) {
			((void(*)(unsigned long, unsigned long))retvals[0])(retvals[2], retvals[3]);
		}
		if (retvals[1]) {
			((void(*)(unsigned long))retvals[1])(retvals[4]);
		}
	}
	rt_make_soft_real_time();

	rt_task_delete(arg->task);
	free(arg);
	return (void *)0;
}
#endif

DECLARE int rt_spset_callback_fun(unsigned int tty, void (*callback_fun)(int, int), int rxthrs, int txthrs)
{
	int ret;
	pthread_t thread;
	struct { int tty; void *callback_fun; int rxthrs, txthrs, code; void *task; } arg = { tty, callback_fun, rxthrs, txthrs, _SPSET_CALLBACK, 0 };
	if ((ret = rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPSET_CALLBACK, &arg).i[LOW]) == EINVAL) {
		void *argp;
		argp = (void *)malloc(sizeof(arg));
		memcpy(argp, &arg, sizeof(arg));
		return pthread_create(&thread, NULL, callback_thread, argp);
	}
	return ret;
}

DECLARE int rt_spset_err_callback_fun(unsigned int tty, void (*err_callback_fun)(int))
{
	int ret;
	pthread_t thread;
	struct { int tty; void *err_callback_fun; int dummy1, dummy2, code; void *task; } arg = { tty, err_callback_fun, 0, 0, _SPSET_ERR_CALLBACK, 0 };
	if ((ret = rtai_lxrt(FUN_EXT_RTAI_SP, SIZARG, _SPSET_ERR_CALLBACK, &arg).i[LOW]) == EINVAL) {
		void *argp;
		argp = (void *)malloc(sizeof(arg));
		memcpy(argp, &arg, sizeof(arg));
		return pthread_create(&thread, NULL, callback_thread, argp);
	}
	return ret;
}

static inline int rt_com_setup(unsigned int tty, int baud, int mode, unsigned int parity, unsigned int stopbits, unsigned int numbits, int fifotrig)
{
	return baud <= 0 ? rt_spclose(tty) : rt_spopen(tty, baud, numbits, stopbits, parity, mode, fifotrig);
}

static inline int rt_com_read(unsigned int tty, char *msg, int msg_size)
{
	int notrd;
	if ((notrd = rt_spread(tty, msg, msg_size)) >= 0) {
		return abs(msg_size) - notrd;
	}
	return notrd;
}
static inline int rt_com_write(unsigned int tty, char *msg, int msg_size)
{
	int notwr;
	if ((notwr = rt_spwrite(tty, msg, msg_size)) >= 0) {
		return abs(msg_size) - notwr;
	}
	return notwr;
}

#define rt_com_clear_input(indx) rt_spclear_rx(indx)
#define rt_com_clear_output(indx) rt_spclear_tx(indx)

#define rt_com_write_modem(indx, mask, op) rt_spset_mcr(indx, mask, op)
#define rt_com_read_modem(indx, mask) rt_spget_msr(indx, mask)

#define rt_com_set_mode(indx, mode) rt_spset_mode(indx, mode)
#define rt_com_set_fifotrig(indx, fifotrig) rt_spset_fifotrig(indx, fifotrig)

#define rt_com_error(indx) rt_spget_err(indx)

#endif

#endif /* _RTAI_SPDRV_LXRT_H_ */
