/*
COPYRIGHT (C) 2002 Thomas Leibner (leibner@t-online.de) (first complete writeup)
              2002 David Schleef (ds@schleef.org) (COMEDI master)
              2002 Lorenzo Dozio (dozio@aero.polimi.it) (made it all work)
              2002 Paolo Mantegazza (mantegazza@aero.polimi.it) (hints/support)

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


#ifndef _RTAI_COMEDI_LXRT_H_
#define _RTAI_COMEDI_LXRT_H_

#include <rtai_declare.h>

#define FUN_COMEDI_LXRT_INDX  9

#define _KCOMEDI_OPEN 			 0
#define _KCOMEDI_CLOSE 			 1
#define _KCOMEDI_LOCK 			 2
#define _KCOMEDI_UNLOCK 		 3
#define _KCOMEDI_CANCEL 		 4
#define _KCOMEDI_REGISTER_CALLBACK 	 5
#define _KCOMEDI_COMMAND 		 6
#define _KCOMEDI_COMMAND_TEST 		 7

/* DEPRECATED function */
#define _KCOMEDI_TRIGGER 		 8

#define _KCOMEDI_DATA_WRITE 		 9
#define _KCOMEDI_DATA_READ 		10
#define _KCOMEDI_DIO_CONFIG 		11
#define _KCOMEDI_DIO_READ 		12
#define _KCOMEDI_DIO_WRITE 		13
#define _KCOMEDI_DIO_BITFIELD 		14
#define _KCOMEDI_GET_N_SUBDEVICES 	15
#define _KCOMEDI_GET_VERSION_CODE 	16
#define _KCOMEDI_GET_DRIVER_NAME 	17
#define _KCOMEDI_GET_BOARD_NAME 	18
#define _KCOMEDI_GET_SUBDEVICE_TYPE 	19
#define _KCOMEDI_FIND_SUBDEVICE_TYPE	20
#define _KCOMEDI_GET_N_CHANNELS 	21
#define _KCOMEDI_GET_MAXDATA 		22
#define _KCOMEDI_GET_N_RANGES 		23
#define _KCOMEDI_DO_INSN 		24
#define _KCOMEDI_DO_INSN_LIST		25  // not yet in kcomedilib... (tl) 
#define _KCOMEDI_POLL 			26

/* DEPRECATED function */
#define _KCOMEDI_GET_RANGETYPE 		27

/* ALPHA functions */
#define _KCOMEDI_GET_SUBDEVICE_FLAGS 	28
#define _KCOMEDI_GET_LEN_CHANLIST 	29
#define _KCOMEDI_GET_KRANGE 		30
#define _KCOMEDI_GET_BUF_HEAD_POS	31
#define _KCOMEDI_SET_USER_INT_COUNT	32
#define _KCOMEDI_MAP 			33
#define _KCOMEDI_UNMAP 			34

/* RTAI specific callbacks from kcomedi to user space */
#define _KCOMEDI_WAIT        		35
#define _KCOMEDI_WAIT_IF     		36
#define _KCOMEDI_WAIT_UNTIL  		37
#define _KCOMEDI_WAIT_TIMED  		38

/* RTAI specific functions to allocate/free comedi_cmd */
#define _KCOMEDI_ALLOC_CMD  		39
#define _KCOMEDI_FREE_CMD  		40

#ifdef __KERNEL__

#include <linux/comedilib.h>

extern int rt_comedi_register_callback(void *dev, unsigned int subdev, unsigned int mask, SEM *sem);
extern unsigned int rt_comedi_wait(SEM *sem, int *semcnt);
extern unsigned int rt_comedi_wait_if(SEM *sem, int *semcnt);
extern unsigned int rt_comedi_wait_until(SEM *sem, RTIME until, int *semcnt);
extern unsigned int rt_comedi_wait_timed(SEM *sem, RTIME delay, int *semcnt);
extern char *rt_comedi_get_driver_name(void *dev, char *name);
extern char *rt_comedi_get_board_name(void *dev, char *name);

#else  /* __KERNEL__ not defined */

#include <string.h>
#include <asm/rtai_lxrt.h>
#define _RTAI_NAM2NUM_H_	/* dirty hack to get rid of the mess of too many nam2num when using libcomedilxrt */
#include <rtai_shm.h>
#include <linux/comedi.h>

extern union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg);

#define SIZARG sizeof(arg)

DECLARE void *comedi_open(const char *filename)
{
	char lfilename[COMEDI_NAMELEN];
        struct { char *minor; } arg = { lfilename };
	strncpy(lfilename, filename, COMEDI_NAMELEN - 1);
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_OPEN, &arg).v[LOW];
}

DECLARE int comedi_close(void *dev)
{
        struct { void *dev; } arg = { dev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_CLOSE, &arg).i[LOW];
}

DECLARE int comedi_lock(void *dev, unsigned int subdev)
{
        struct { void *dev; unsigned int subdev; } arg = { dev, subdev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_LOCK, &arg).i[LOW];
}

DECLARE int comedi_unlock(void *dev, unsigned int subdev)
{
        struct { void *dev; unsigned int subdev; } arg = { dev, subdev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_UNLOCK, &arg).i[LOW];
}

DECLARE int comedi_cancel(void *dev, unsigned int subdev)
{
        struct { void *dev; unsigned int subdev; } arg = { dev, subdev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_CANCEL, &arg).i[LOW];
}

DECLARE int rt_comedi_register_callback(void *dev, unsigned int subdev, unsigned int mask, SEM *sem)
{
        struct { void *dev; unsigned int subdev; unsigned int mask; SEM *sem; } arg = { dev, subdev, mask, sem };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_REGISTER_CALLBACK, &arg).i[LOW];
}

#define comedi_register_callback(dev, subdev, mask, cb, arg)  rt_comedi_register_callback(dev, subdev, mask, arg)

DECLARE unsigned int rt_comedi_wait(SEM *sem, int *semcnt)
{
	int lsemcnt;
	unsigned int retval;
        struct { SEM *sem; int *semcnt; int size; } arg = { sem, &lsemcnt, sizeof(int *) };
        retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_WAIT, &arg).i[LOW];
	if (semcnt) {
		*semcnt = lsemcnt;
	}
        return retval;
}

DECLARE unsigned int rt_comedi_wait_if(SEM *sem, int *semcnt)
{
	int lsemcnt;
	unsigned int retval;
        struct { SEM *sem; int *semcnt; int size; } arg = { sem, &lsemcnt, sizeof(int *) };
        retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_WAIT_IF, &arg).i[LOW];
	if (semcnt) {
		*semcnt = lsemcnt;
	}
        return retval;
}

DECLARE unsigned int rt_comedi_wait_until(SEM *sem, RTIME until, int *semcnt)
{
	int lsemcnt;
	unsigned int retval;
        struct { SEM *sem; RTIME until; int *semcnt; int size; } arg = { sem, until, &lsemcnt, sizeof(int *) };
        retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_WAIT_UNTIL, &arg).i[LOW];
	if (semcnt) {
		*semcnt = lsemcnt;
	}
        return retval;
}

DECLARE unsigned int rt_comedi_wait_timed(SEM *sem, RTIME delay, int *semcnt)
{
	int lsemcnt;
	unsigned int retval;
        struct { SEM *sem; RTIME delay; int *semcnt; int size; } arg = { sem, delay, &lsemcnt, sizeof(int *) };
        retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_WAIT_TIMED, &arg).i[LOW];
	if (semcnt) {
		*semcnt = lsemcnt;
	}
        return retval;
}

DECLARE int comedi_command(void *dev, comedi_cmd *cmd)
{
        struct { void *dev; comedi_cmd *cmd; } arg = { dev, cmd };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_COMMAND, &arg).i[LOW];
}

DECLARE int comedi_command_test(void *dev, comedi_cmd *cmd)
{
        struct { void *dev; comedi_cmd *cmd; } arg = { dev, cmd };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_COMMAND_TEST, &arg).i[LOW];
}

DECLARE int comedi_trigger(void *dev, unsigned int subdev, comedi_trig *it)
{
	return -1;
}

DECLARE int comedi_data_write(void *dev, unsigned int subdev, unsigned int chan, unsigned int range, unsigned int aref, lsampl_t data)
{
	struct { void *dev; unsigned int subdev; unsigned int chan; unsigned int range; unsigned int aref; lsampl_t data; } arg = { dev, subdev, chan, range, aref, data };
	return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DATA_WRITE, &arg).i[LOW];
}

DECLARE int comedi_data_read(void *dev, unsigned int subdev, unsigned int chan, unsigned int range, unsigned int aref, lsampl_t *data)
{
	int retval;
	lsampl_t ldata;
	struct { void *dev; unsigned int subdev; unsigned int chan; unsigned int range; unsigned int aref; lsampl_t *data; } arg = { dev, subdev, chan, range, aref, &ldata };
	retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DATA_READ, &arg).i[LOW];
	memcpy(data, &ldata, sizeof(lsampl_t));
	return retval;
}

DECLARE int comedi_dio_config(void *dev, unsigned int subdev, unsigned int chan, unsigned int io)
{
	struct { void *dev; unsigned int subdev; unsigned int chan; unsigned int io; } arg = { dev, subdev, chan, io };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DIO_CONFIG, &arg).i[LOW];
}

DECLARE int comedi_dio_read(void *dev, unsigned int subdev, unsigned int chan, unsigned int *val)
{
        int retval;
	unsigned int lval;
        struct { void *dev; unsigned int subdev; unsigned int chan; unsigned int *val; } arg = { dev, subdev, chan, &lval };
	retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DIO_READ, &arg).i[LOW];
	*val = lval;
	return retval;
}

DECLARE int comedi_dio_write(void *dev, unsigned int subdev, unsigned int chan, unsigned int val)
{
        struct { void *dev; unsigned int subdev; unsigned int chan; unsigned int val; } arg = { dev, subdev, chan, val };
	return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DIO_WRITE, &arg).i[LOW];
}

DECLARE int comedi_dio_bitfield(void *dev, unsigned int subdev, unsigned int mask, unsigned int *bits)
{
        int retval;
	unsigned int lbits;
        struct { void *dev; unsigned int subdev; unsigned int mask; unsigned int *bits; } arg = { dev, subdev, mask, &lbits };
	retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DIO_BITFIELD, &arg).i[LOW];
	*bits = lbits;
	return retval;
}

DECLARE int comedi_get_n_subdevices(void *dev)
{
	struct { void *dev;} arg = { dev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_N_SUBDEVICES, &arg).i[LOW];
}

DECLARE int comedi_get_version_code(void *dev)
{
	struct { void *dev;} arg = { dev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_VERSION_CODE, &arg).i[LOW];
}

DECLARE char *rt_comedi_get_driver_name(void *dev, char *name)
{
	void *p;
        char lname[COMEDI_NAMELEN];
        struct { void *dev; char *name; } arg = { dev, lname };
	if ((p = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_DRIVER_NAME, &arg).v[LOW])) {
		strncpy(name, lname, COMEDI_NAMELEN);
		return name;
	}
	return 0;
}

DECLARE char *rt_comedi_get_board_name(void *dev, char *name)
{
	void *p;
        char lname[COMEDI_NAMELEN];
        struct { void *dev; char *name; } arg = { dev, lname };
	if ((p = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_BOARD_NAME, &arg).v[LOW])) {
		strncpy(name, lname, COMEDI_NAMELEN);
		return name;
	}
	return 0;
}

DECLARE int comedi_get_subdevice_type(void *dev, unsigned int subdev)
{
        struct { void *dev; unsigned int subdev; } arg = { dev, subdev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_SUBDEVICE_TYPE, &arg).i[LOW];
}

DECLARE int comedi_find_subdevice_by_type(void *dev, int type, unsigned int subd)
{
        struct { void *dev; int type; unsigned int subd; } arg = { dev, type, subd };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_FIND_SUBDEVICE_TYPE, &arg).i[LOW];
}

DECLARE int comedi_get_n_channels(void *dev, unsigned int subdev)
{
        struct { void *dev; unsigned int subdev; } arg = { dev, subdev };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_N_CHANNELS, &arg).i[LOW];
}

DECLARE lsampl_t comedi_get_maxdata(void *dev, unsigned int subdev, unsigned int chan)
{
        struct { void *dev; unsigned int subdev; unsigned int chan;} arg = { dev, subdev, chan };
        return (lsampl_t)rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_MAXDATA, &arg).i[LOW];
}

DECLARE int comedi_get_n_ranges(void *dev, unsigned int subdev, unsigned int chan)
{
        struct { void *dev; unsigned int subdev; unsigned int chan;} arg = { dev, subdev, chan };
        return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_N_RANGES, &arg).i[LOW];
}

DECLARE int comedi_do_insn(void *dev, comedi_insn *insn)
{
	int retval;
	comedi_insn linsn = *insn;
	struct { void *dev; comedi_insn *insn; } arg = { dev, &linsn };
	retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_DO_INSN, &arg).i[LOW];
	memcpy(insn, &linsn, sizeof(comedi_insn));
	return retval;
}

DECLARE int comedi_poll(void *dev, unsigned int subdev)
{
	struct { void *dev; unsigned int subdev; } arg = { dev, subdev };
	return rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_POLL, &arg).i[LOW];
}

DECLARE int comedi_get_krange(void *dev, unsigned int subdev, unsigned int chan, unsigned int range, comedi_krange *krange)
{
	int retval;
	comedi_krange lkrange;
	struct { void *dev; unsigned int subdev; unsigned int chan; unsigned int range; comedi_krange *krange; } arg = { dev, subdev, chan, range, &lkrange };
	retval = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_GET_KRANGE, &arg).i[LOW];
	memcpy(krange, &lkrange, sizeof(comedi_krange));
	return retval;
}

DECLARE comedi_cmd *rt_comedi_alloc_cmd(unsigned int **chanlist, unsigned int chanlist_len, sampl_t **data,  unsigned int data_len)
{
	unsigned long ofst[2], name;
	comedi_cmd *cmd;
	void *p;

	struct { unsigned int chanlist_len, data_len; unsigned long *ofst; } arg = { chanlist_len, data_len, ofst };
	name = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_ALLOC_CMD, &arg).i[LOW];
	cmd = p = rtai_malloc(name, 1);
        *chanlist = cmd->chanlist = (void *)((char *)p + ofst[0]);
        *data = cmd->data = (void *)((char *)p + ofst[1]);
	return cmd;
}

DECLARE void rt_comedi_free_cmd(void *cmd)
{
	unsigned long name;
        struct { void *cmd; } arg = { cmd };
        name = rtai_lxrt(FUN_COMEDI_LXRT_INDX, SIZARG, _KCOMEDI_FREE_CMD, &arg).i[LOW];
	rtai_free(name, cmd);
}

#endif /* #ifdef __KERNEL__ */

#endif /* #ifndef _RTAI_COMEDI_LXRT_H_ */
