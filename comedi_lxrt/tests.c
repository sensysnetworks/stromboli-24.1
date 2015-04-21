/*
COPYRIGHT (C) 2002 Lorenzo Dozio (dozio@aero.polimi.it)

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


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define KEEP_STATIC_INLINE		/* undef this to use libcomedilxrt */
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <rtai_comedi_lxrt.h>		/* comment this when using libcomedilxrt */
//#include <rtai_comedi_lxrtlib.h>	/* include this to use libcomedilxrt */

char *subdevice_types[]={
        "unused",
        "analog input",
        "analog output",
        "digital input",
        "digital output",
        "digital I/O",
        "counter",
        "timer",
        "memory",
        "calibration",
        "processor"
};

int main(int argc, char **argv)
{
	RT_TASK *comedi_task;
	void *dev;
	int i, n_subdevs, type;
	int subdev_ai, subdev_ao, subdev_dio;
	lsampl_t data;
	int n, nch, semcnt;
	char name[50];
	SEM *sem;
	
 	if (!(comedi_task = rt_task_init(nam2num("COMEDI"), 1, 0, 0))) {
		printf("CANNOT INIT COMEDI TASK\n");
		exit(1);
	}
	sem = rt_sem_init(nam2num("SEM"), 0);
	rt_set_oneshot_mode();
	start_rt_timer(0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	dev = comedi_open("/dev/comedi0");

	printf("\n OVERALL INFO:\n");
	printf("   Version code : 0x%06x\n", comedi_get_version_code(dev));
	rt_comedi_get_board_name(dev, name);
	printf("   Board name   : %s\n", name);
	rt_comedi_get_driver_name(dev, name);
	printf("   Driver name  : %s\n", name);
	printf("   Number of subdevices : %d\n", n_subdevs = comedi_get_n_subdevices(dev));

	for (i = 0; i < 3; i++) {
		printf("\n Subdevice : %d\n", i);
		type = comedi_get_subdevice_type(dev, i);
		printf(" Type : %d (%s)\n", type, subdevice_types[type]);
		printf(" Number of channels : %d\n", nch = comedi_get_n_channels(dev, i));
		printf(" Maxdata : %d\n", comedi_get_maxdata(dev, i, 0));
		printf(" Number of ranges : %d\n", comedi_get_n_ranges(dev, i, 0));
	}

	subdev_ai = comedi_find_subdevice_by_type(dev, COMEDI_SUBD_AI, 0);
	subdev_ao = comedi_find_subdevice_by_type(dev, COMEDI_SUBD_AO, 0);
	subdev_dio = comedi_find_subdevice_by_type(dev, COMEDI_SUBD_DIO, 0);
	
	printf("\n Synchronous test ...");
	fflush(stdout);

	comedi_lock(dev, subdev_ai);

	rt_make_hard_real_time();

	for (n = 0; n < 4095; n++) {
		comedi_data_write(dev, subdev_ao, 1, 0, AREF_GROUND, n);
		rt_comedi_wait_timed(sem, nano2count(100000), &semcnt);
		comedi_data_read(dev, subdev_ai, 0, 0, AREF_GROUND, &data);
		comedi_data_write(dev, subdev_ao, 0, 0, AREF_GROUND, data);
	}
	comedi_data_write(dev, subdev_ao, 0, 0, AREF_GROUND, 2048);
	comedi_data_write(dev, subdev_ao, 1, 0, AREF_GROUND, 2048);

	rt_make_soft_real_time();

	printf(" OK.\n");

	comedi_unlock(dev, subdev_ai);

/*
	printf("\n  command before testing : \n");
	dump_cmd(cmd);
	ret = comedi_command_test(dev, cmd);
	printf("\n  First test returned %d (%s)\n", ret, cmdtest_messages[ret]);
	dump_cmd(cmd);
	ret = comedi_command_test(dev, cmd);
	printf("\n  Second test returned %d (%s)\n\n", ret, cmdtest_messages[ret]);
	if (ret != 0) {
		dump_cmd(cmd);
		printf(" ERROR PREPARING COMMAND\n");
		exit(1);
	}
*/

	comedi_close(dev);

	rt_sem_delete(sem);
	stop_rt_timer();
	rt_task_delete(comedi_task);

	return 0;
}
