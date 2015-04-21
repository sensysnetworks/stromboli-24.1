/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)
                    Giuseppe Renoldi (giuseppe@renoldi.org)

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


#ifndef RTAI_SPDRV_H
#define RTAI_SPDRV_H

#define TTY0  0
#define TTY1  1
#define COM1  TTY0
#define COM2  TTY1

#define RT_SP_NO_HAND_SHAKE  0x00
#define RT_SP_DSR_ON_TX      0x01
#define RT_SP_HW_FLOW	     0x02

#define RT_SP_PARITY_EVEN    0x18
#define RT_SP_PARITY_NONE    0x00
#define RT_SP_PARITY_ODD     0x08
#define RT_SP_PARITY_HIGH    0x28
#define RT_SP_PARITY_LOW     0x38

#define RT_SP_FIFO_DISABLE   0x00
#define RT_SP_FIFO_SIZE_1    0x00
#define RT_SP_FIFO_SIZE_4    0x40
#define RT_SP_FIFO_SIZE_8    0x80
#define RT_SP_FIFO_SIZE_14   0xC0

#define RT_SP_FIFO_SIZE_DEFAULT  RT_SP_FIFO_SIZE_8

#define RT_SP_DTR            0x01
#define RT_SP_RTS            0x02 

#define RT_SP_CTS            0x10
#define RT_SP_DSR            0x20
#define RT_SP_RI             0x40
#define RT_SP_DCD            0x80

#define RT_SP_BUFFER_FULL    0x01
#define RT_SP_OVERRUN_ERR    0x02
#define RT_SP_PARITY_ERR     0x04
#define RT_SP_FRAMING_ERR    0x08
#define RT_SP_BREAK          0x10
#define RT_SP_BUFFER_OVF     0x20

#define DELAY_FOREVER  0x3FFFFFFFFFFFFFFFLL

#ifdef __KERNEL__

#include <rtai.h>

extern int rt_spopen(unsigned int tty, unsigned int baud, unsigned int numbits, unsigned int stopbits, unsigned int parity, int mode, int fifotrig);

extern int rt_spclose(unsigned int tty);

extern int rt_spread(unsigned int tty, char *msg, int msg_size);

extern int rt_spevdrp(unsigned int tty, char *msg, int msg_size);

extern int rt_spwrite(unsigned int tty, char *msg, int msg_size);

extern int rt_spget_rxavbs(unsigned int tty);

extern int rt_spget_txfrbs(unsigned int tty);

extern int rt_spclear_rx(unsigned int tty);

extern int rt_spclear_tx(unsigned int tty);

extern int rt_spset_mcr(unsigned int tty, int mask, int setbits);

extern int rt_spget_msr(unsigned int tty, int mask);

extern int rt_spset_mode(unsigned int tty, int mode);

extern int rt_spset_fifotrig(unsigned int tty, int fifotrig);

extern int rt_spget_err(unsigned int tty);

extern int rt_spset_callback_fun(unsigned int tty, void (*callback_fun)(int, int), int rxthrs, int txthrs);

extern int rt_spset_thrs(unsigned int tty, int rxthrs, int txthrs);

extern int rt_spset_err_callback_fun(unsigned int tty, void (*err_callback_fun)(int));

extern int rt_spset_callback_fun_usr(unsigned int tty, unsigned long callback_fun, int rxthrs, int txthrs, int code, void *task);

extern int rt_spset_err_callback_fun_usr(unsigned int tty, unsigned long err_callback_fun, int dummy1, int dummy2, int code, void *task);

extern void rt_spwait_usr_callback(unsigned int tty, unsigned long *retvals);

extern int rt_spread_timed(unsigned int tty, char *msg, int msg_size, RTIME delay);

extern int rt_spwrite_timed(unsigned int tty, char *msg, int msg_size, RTIME delay);

//
// rt_com compatibility functions
// 

static inline int rt_com_setup(unsigned int tty, int baud, int mode, unsigned int parity, unsigned int stopbits, unsigned int numbits, int fifotrig)
{
	return baud <= 0 ? rt_spclose(tty) : rt_spopen(tty, baud, numbits, stopbits, parity, mode, fifotrig);
}

static inline int rt_com_read(unsigned int tty, char *msg, int msg_size)
{
	int notrd;
	if ((notrd = rt_spread(tty, msg, msg_size)) >= 0) {
		return msg_size - notrd;
	}
	return notrd;
}
static inline int rt_com_write(unsigned int tty, char *msg, int msg_size)
{
	int notwr;
	if ((notwr = rt_spwrite(tty, msg, msg_size)) >= 0) {
		return msg_size - notwr;
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

#endif /* RTAI_SPDRV_H */
