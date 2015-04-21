
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "control.h"

MODULE_LICENSE("GPL");

EXPORT_NO_SYMBOLS;

static RT_TASK tasks[2];

static char *data[] = {"Frank ", "Zappa "};

static void thread_code(int fifo)
{
	int taskno = fifo;
	struct my_msg_struct msg;
	while (1) {
		if (rtf_get(taskno + COMMAND_FIFO + 1, &msg, sizeof(msg)) == sizeof(msg)) {
			rt_printk("Task %d: executing the \"%d\" command to task %d; period %d (ms)\n", taskno,  msg.command, msg.task, msg.period);
			switch (msg.command) {
				case START_TASK:
					rt_task_make_periodic_relative_ns(rt_whoami(), 0, msg.period*1000000);

					break;
				case STOP_TASK:
					rt_task_suspend(rt_whoami());
					break;
				default:
					rt_printk("RTL task: bad command\n");
			}
		}
		rtf_put(fifo, data[fifo], 6);
		rt_task_wait_period();
	}
}

static int my_handler(unsigned int fifo, int rw)
{
	struct my_msg_struct msg;
	int err;

	if (rw == 'r') {
		return -EINVAL;
	}
	while ((err = rtf_get(COMMAND_FIFO, &msg, sizeof(msg))) == sizeof(msg)) {
		rtf_put (msg.task + COMMAND_FIFO + 1, &msg, sizeof(msg));
		rt_printk("FIFO handler: sending the \"%d\" command to task %d; period %d (ms)\n", msg.command, msg.task, msg.period);
		rt_task_resume(&tasks[msg.task]);
	}
	if (err != 0) {
		return -EINVAL;
	}
	return 0;
}


int init_module(void)
{
 	rtf_create(0, 4000);
	rtf_create(1, 4000);
	rtf_create(2, 200);
	rtf_create(3, 100);
	rtf_create(4, 100);
	rt_task_init(&tasks[0], thread_code, 0, 1000, 0, 0, 0);
	rt_task_init(&tasks[1], thread_code, 1, 1000, 1, 0, 0);
	rtf_create_handler(COMMAND_FIFO, X_FIFO_HANDLER(my_handler));
	rt_set_oneshot_mode();
	start_rt_timer_ns(10000000);
	return 0;
}


void cleanup_module(void)
{
	rtf_destroy(0);
	rtf_destroy(1);
	rtf_destroy(2);
	rtf_destroy(3);
	rtf_destroy(4);
	stop_rt_timer();
	rt_task_delete(&tasks[0]);
	rt_task_delete(&tasks[1]);
}
