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

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <rtai_comedi_lxrt.h>

#define N_CHANS		1
#define BUFSZ		20

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

char *cmdtest_messages[]={
        "success",
        "invalid source",
        "source conflict",
        "invalid argument",
        "argument conflict",
        "invalid chanlist",
};

char *cmd_src(int src,char *buf)
{
        buf[0]=0;

        if(src&TRIG_NONE)strcat(buf,"none|");
        if(src&TRIG_NOW)strcat(buf,"now|");
        if(src&TRIG_FOLLOW)strcat(buf, "follow|");
        if(src&TRIG_TIME)strcat(buf, "time|");
        if(src&TRIG_TIMER)strcat(buf, "timer|");
        if(src&TRIG_COUNT)strcat(buf, "count|");
        if(src&TRIG_EXT)strcat(buf, "ext|");
        if(src&TRIG_INT)strcat(buf, "int|");
#ifdef TRIG_OTHER
        if(src&TRIG_OTHER)strcat(buf, "other|");
#endif

        if(strlen(buf)==0){
                sprintf(buf,"unknown(0x%08x)",src);
        }else{
                buf[strlen(buf)-1]=0;
        }

        return buf;
}

void dump_cmd(comedi_cmd *cmd)
{
        char buf[100];

        printf("start:      %-8s %d\n",
                cmd_src(cmd->start_src,buf),
                cmd->start_arg);

        printf("scan_begin: %-8s %d\n",
                cmd_src(cmd->scan_begin_src,buf),
                cmd->scan_begin_arg);

        printf("convert:    %-8s %d\n",
                cmd_src(cmd->convert_src,buf),
                cmd->convert_arg);

        printf("scan_end:   %-8s %d\n",
                cmd_src(cmd->scan_end_src,buf),
                cmd->scan_end_arg);

        printf("stop:       %-8s %d\n",
                cmd_src(cmd->stop_src,buf),
                cmd->stop_arg);
}

int main(int argc, char **argv)
{
	RT_TASK *comedi_task;
	void *dev;
	int i, n_subdevs, type;
	int subdev_ai, subdev_ao, subdev_dio;
	int n, nch, semcnt;
	char name[50];
	SEM *sem;
	comedi_cmd *cmd;
	unsigned int *chanlist;
	sampl_t *buf;
	unsigned int mask;
	int ret;
	
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
	
	printf("\n Asynchronous test (with comedi commands).\n");

	comedi_lock(dev, subdev_ai);

	comedi_register_callback(dev, subdev_ai, COMEDI_CB_EOS, 0, sem);

	cmd = rt_comedi_alloc_cmd(&chanlist, N_CHANS, &buf, BUFSZ);

/* the subdevice that the command is sent to */
	cmd->subdev = subdev_ai;

/* flags */
	cmd->flags = 0;

/* start of acquisition */
	cmd->start_src = TRIG_NOW;
	cmd->start_arg = 0;

/* timing of the beginning of each scan */
	cmd->scan_begin_src = TRIG_TIMER;
	cmd->scan_begin_arg = 100000;

/* timing between each sample in a scan */
	cmd->convert_src = TRIG_TIMER;
	cmd->convert_arg = 1;

/* end of each scan */
	cmd->scan_end_src = TRIG_COUNT;
	cmd->scan_end_arg = N_CHANS;

/* end of acquisition */
	cmd->stop_src = TRIG_COUNT;
	cmd->stop_arg = BUFSZ/2;

/* the channel list */
	cmd->chanlist = chanlist;
	cmd->chanlist_len = N_CHANS;
	chanlist[0] = CR_PACK(0,0,0);

	cmd->data = buf;
	cmd->data_len = BUFSZ*sizeof(sampl_t);

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

	comedi_command(dev, cmd);

//	rt_make_hard_real_time();
	for (n = 0; n < BUFSZ/2; n++) {
		mask = rt_comedi_wait_timed(sem, nano2count(1000000), &semcnt);
		printf("#: %d, #SMPV: %d, MASK: %x, SEMCNT: %d (%s)\n", n, buf[n], mask, semcnt, semcnt >= SEM_TIMOUT ? "TIMEOUT" : "TRUE COUNT");
	}
	rt_make_soft_real_time();

	comedi_cancel(dev, subdev_ai);

	rt_comedi_free_cmd(cmd);

	comedi_unlock(dev, subdev_ai);
	comedi_close(dev);

	rt_sem_delete(sem);
	stop_rt_timer();
	rt_task_delete(comedi_task);

	return 0;
}
