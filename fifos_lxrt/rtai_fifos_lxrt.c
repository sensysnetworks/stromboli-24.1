/*
COPYRIGHT (C) 2002  Thomas Leibner (leibner@t-online.de)
              2002  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_lxrt.h>
#include <rtai_fifos_lxrt.h>

#define MODULE_NAME "rtai_fifos_lxrt"

static struct rt_fun_entry rtai_fifos_fun[] = {
	[_CREATE]       = { 0, rtf_create },
	[_DESTROY]      = { 0, rtf_destroy },
	[_PUT]          = { 0, rtf_put },
	[_GET]          = { 0, rtf_get },
	[_RESET]        = { 0, rtf_reset },
	[_RESIZE]       = { 0, rtf_resize },
	[_SEM_INIT]     = { 0, rtf_sem_init },
	[_SEM_DESTRY]   = { 0, rtf_sem_destroy },
	[_SEM_POST]     = { 0, rtf_sem_post },
	[_SEM_TRY]      = { 0, rtf_sem_trywait },
#ifdef CONFIG_RTAI_RTF_NAMED
	[_CREATE_NAMED] = { 0, rtf_create_named },
	[_GETBY_NAME]   = { 0, rtf_getfifobyname }
#endif
};

int init_module(void)
{
	if (set_rt_fun_ext_index(rtai_fifos_fun, FUN_FIFOS_LXRT_INDX)) {
		printk("LXRT EXTENSION SLOT FOR FIFOS (%d) ALREADY USED\n", FUN_FIFOS_LXRT_INDX);
		return -EACCES;
	}			
	printk("%s: LOADED.\n", MODULE_NAME);
	return(0);
}

void cleanup_module(void)
{
	reset_rt_fun_ext_index(rtai_fifos_fun, FUN_FIFOS_LXRT_INDX);
	printk("%s: UNLOADED.\n", MODULE_NAME);
}
