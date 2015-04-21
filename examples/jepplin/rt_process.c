
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

MODULE_LICENSE("GPL");

#define ONE_SHOT

static char *strs[] = { "Joey    ", "Johnny  ", "Dee Dee ", "Marky   " };
static char sync_str[] = "sync\n";

#define NUM_TASKS (sizeof(strs)/sizeof(char *))

static RT_TASK start_task, tasks[NUM_TASKS];

static SEM sems[NUM_TASKS], sync_sem, prio_sem;

static MBX mbx_in, mbx_out;

#define DBG_PRINT_SETUP         SEM print_sem;
#define DBG_PRINT_INIT          rt_typed_sem_init(&print_sem, 1, BIN_SEM);
#define TAKE_PRINT              rt_sem_wait(&print_sem);
#define GIVE_PRINT              rt_sem_signal(&print_sem);

DBG_PRINT_SETUP

/*
 *  Each task waits to receive the semaphore, prints its string, and
 *  passes the semaphore to the next task.  Then it sends a sync semaphore,
 *  and waits for another semaphore, and this time displays it in
 *  priority order.  Finally, message queues are tested.
 */

static RTIME checkt;
static int checkj;

static void task_code(int task_no)
{
	int i, ret;
	char buf[9];
	buf[8] = 0;
	for (i = 0; i < 5; ++i)	{
		rt_sem_wait(&sems[task_no]);
		TAKE_PRINT; 
		rt_printk(strs[task_no]);
		GIVE_PRINT;
		if (task_no == NUM_TASKS-1) {
			rt_printk("\n");
		}
		rt_sem_signal(&sems[(task_no + 1) % NUM_TASKS]);
	}
	rt_sem_signal(&sync_sem);
	rt_sem_wait(&prio_sem);
	TAKE_PRINT; 
	rt_printk(strs[task_no]);
	GIVE_PRINT;
	rt_sleep(nano2count(1000000000LL));
	rt_sem_wait_timed(&prio_sem, nano2count((task_no + 1)*1000000000LL));
	TAKE_PRINT; 
	rt_printk("sem timeout, task %d, %s\n", task_no, strs[task_no]);
	GIVE_PRINT;
	rt_sem_signal(&sync_sem);

/* message queue stuff */
	if ((ret = rt_mbx_receive(&mbx_in, buf, 8)) != 0) {
		rt_printk("rt_mbx_receive() failed with %d\n", ret);
	}
	TAKE_PRINT; 
	rt_printk("\nreceived by task %d ", task_no);
	rt_printk(buf); 
	GIVE_PRINT;
	rt_mbx_send(&mbx_out, strs[task_no], 8);
/* test receive timeout */
	rt_sem_wait(&sync_sem);
	if (rt_mbx_receive_timed(&mbx_in, buf, 8, nano2count((task_no + 1)*1000000000LL))) {
		TAKE_PRINT;
		rt_printk("mbx timeout, task %d, %s\n", task_no, strs[task_no]);
		GIVE_PRINT;
	}

	rt_printk("\ntask %d complete\n", task_no); 
}

/*
 * initialization task
 */
static void start_task_code(int notused)
{
	int i;
	char buf[9];
	buf[8] = 0;
  /* create the sync semaphore */
	rt_sem_init(&sync_sem, 0);
  /* create the priority-test semaphore */
	rt_sem_init(&prio_sem, 0);
  /* pass the semaphore to the first task */
	rt_sem_signal(&sems[0]);
  /* wait for each task to send the sync semaphore */
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_sem_wait(&sync_sem);
	}
	rt_printk(sync_str);
  /* post the priority-test semaphore -- the tasks should then run */
  /* in priority order */
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_sem_signal(&prio_sem);
	}
	rt_printk("\n");
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_sem_wait(&sync_sem);
	}
	rt_printk(sync_str);

  /* now, test message queues */
	TAKE_PRINT; 
	rt_printk("testing message queues\n");
	GIVE_PRINT;
	for (i = 0; i < NUM_TASKS; ++i) {
		if (rt_mbx_send(&mbx_in, strs[i], 8)) {
			rt_printk("rt_mbx_send() failed\n");
		}
	}
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_mbx_receive(&mbx_out, buf, 8);
		TAKE_PRINT; 
		rt_printk("\nreceived from mbx_out: %s", buf); 
		GIVE_PRINT;
	}
	rt_printk("\n");
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_sem_signal(&sync_sem);
	}
	TAKE_PRINT; 
	rt_printk("\ninit task complete\n"); 
	GIVE_PRINT;

  /* nothing more for this task to do */
}

int init_module(void)
{
	int i;
	DBG_PRINT_INIT;
	if (rt_task_init(&start_task, start_task_code, 0, 10000, 10, 0, 0) != 0) {
		printk("Could not start init task\n");
	}
	if (rt_mbx_init(&mbx_in, NUM_TASKS*8) || rt_mbx_init(&mbx_out, NUM_TASKS*8)) {
		printk("could not create message queue\n");
		return 1;
	}
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_sem_init(&sems[i], 0);
		if (rt_task_init(&tasks[i], task_code, i, 3000, NUM_TASKS - i, 0, 0) != 0) {
			printk("rt_task_ipc_init failed\n");
			return 1;
		}
	}     
#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	start_rt_timer_ns(10000000);
	checkt = rdtsc();
	checkj = jiffies;
	rt_task_resume(&start_task);
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_task_resume(&tasks[i]);
	}     
	return 0;
}

void cleanup_module(void)
{
	int i;
	checkt = rdtsc() - checkt;
	checkj = jiffies - checkj;
	stop_rt_timer();
	printk("\n(JIFFIES COUNT CHECK: TRUE = %d, LINUX = %d)\n", (int)llimd(checkt, 100, CPU_FREQ), checkj);
	rt_task_delete(&start_task);
	for (i = 0; i < NUM_TASKS; ++i) {
		rt_task_delete(&tasks[i]);
		rt_sem_delete(&sems[i]);
	}
	rt_sem_delete(&sync_sem);
	rt_mbx_delete(&mbx_in);
	rt_mbx_delete(&mbx_out);
}
