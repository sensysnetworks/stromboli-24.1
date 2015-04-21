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


#ifndef _RTAI_COMEDI_LXRTLIB_H_
#define _RTAI_COMEDI_LXRTLIB_H_

#include <linux/comedi.h>

extern void *comedi_open(const char *filename);
extern int comedi_close(void *dev);
extern int comedi_lock(void *dev, unsigned int subdev);
extern int comedi_unlock(void *dev, unsigned int subdev);
extern int comedi_cancel(void *dev, unsigned int subdev);
extern int rt_comedi_register_callback(void *dev, unsigned int subdev, unsigned int mask, SEM *sem);
#define comedi_register_callback(dev, subdev, mask, cb, arg)  rt_comedi_register_callback(dev, subdev, mask, arg);
extern unsigned int rt_comedi_wait(SEM *sem, int *semcnt);
extern unsigned int rt_comedi_wait_if(SEM *sem, int *semcnt);
extern unsigned int rt_comedi_wait_until(SEM *sem, RTIME until, int *semcnt);
extern unsigned int rt_comedi_wait_timed(SEM *sem, RTIME delay, int *semcnt);
extern int comedi_command(void *dev, comedi_cmd *cmd);
extern int comedi_command_test(void *dev, comedi_cmd *cmd);
extern int comedi_trigger(void *dev, unsigned int subdev, comedi_trig *it);
extern int comedi_data_write(void *dev, unsigned int subdev, unsigned int chan, unsigned int range, unsigned int aref, lsampl_t data);
extern int comedi_data_read(void *dev, unsigned int subdev, unsigned int chan, unsigned int range, unsigned int aref, lsampl_t *data);
extern int comedi_dio_config(void *dev, unsigned int subdev, unsigned int chan, unsigned int io);
extern int comedi_dio_read(void *dev, unsigned int subdev, unsigned int chan, unsigned int *val);
extern int comedi_dio_write(void *dev, unsigned int subdev, unsigned int chan, unsigned int val);
extern int comedi_dio_bitfield(void *dev, unsigned int subdev, unsigned int mask, unsigned int *bits);
extern int comedi_get_n_subdevices(void *dev);
extern int comedi_get_version_code(void *dev);
extern char *rt_comedi_get_driver_name(void *dev, char *name);
extern char *rt_comedi_get_board_name(void *dev, char *name);
extern int comedi_get_subdevice_type(void *dev, unsigned int subdev);
extern int comedi_find_subdevice_by_type(void *dev, int type, unsigned int subd);
extern int comedi_get_n_channels(void *dev, unsigned int subdev);
extern lsampl_t comedi_get_maxdata(void *dev, unsigned int subdev, unsigned int chan);
extern int comedi_get_n_ranges(void *dev, unsigned int subdev, unsigned int chan);
extern int comedi_do_insn(void *dev, comedi_insn *insn);
extern int comedi_poll(void *dev, unsigned int subdev);
extern comedi_cmd *rt_comedi_alloc_cmd(unsigned int **chanlist, unsigned int chanlist_len, sampl_t **data,  unsigned int data_len);
extern void rt_comedi_free_cmd(void *cmd);

#endif /* #ifndef _RTAI_COMEDI_LXRTLIB_H_ */
