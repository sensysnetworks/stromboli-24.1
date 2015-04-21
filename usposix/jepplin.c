#include <stdlib.h>
#include <sys/mman.h>

#define KEEP_STATIC_INLINE
#include <rtai_usp_posix.h>
#include <rtai_pqueue.h>

#define MAKE_IT_HARD
#ifdef MAKE_IT_HARD
#define RT_MAKE_HARD_REAL_TIME() do { rt_make_hard_real_time(); } while (0)
#define DISPLAY  rt_printk
#else
#define RT_MAKE_HARD_REAL_TIME()
#define DISPLAY  printf
#endif

static char *strs[] = { "Joey ", "Johnny ", "Dee Dee ", "Marky " };
static char sync_str[] = "sync\n\n";
#define NUM_TASKS  (sizeof(strs)/sizeof(char *))

static pthread_t start_thread, thread[NUM_TASKS];

static sem_t sems[NUM_TASKS], sync_sem, prio_sem;
static pthread_barrier_t barrier;

static pthread_mutex_t print_mtx;
#define PRINT_LOCK    pthread_mutex_lock_rt(&print_mtx);
#define PRINT_UNLOCK  pthread_mutex_unlock_rt(&print_mtx);

#define MAX_MSG_SIZE  10
static MQ_ATTR mqattrs = { NUM_TASKS, MAX_MSG_SIZE, 0, 0};

/*
 *  Each task waits to receive the semaphore, prints its string, and
 *  passes the semaphore to the next task.  Then it sends a sync semaphore,
 *  and waits for another semaphore, and this time displays it in
 *  priority order.  Finally, message queues are tested.
 */
static void *task_code(int task_no)
{
	int i;
	char buf[MAX_MSG_SIZE];
	struct timespec t;
	static mqd_t mq_in, mq_out;

	rt_task_init_schmod(nam2num("TSKOD0") + task_no, NUM_TASKS - task_no, 0, 0, SCHED_FIFO, 0xF);
	mq_in  = mq_open("mq_in", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &mqattrs);
	mq_out = mq_open("mq_out", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &mqattrs);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_barrier_wait_rt(&barrier);
	for (i = 0; i < 5; ++i) {
		sem_wait_rt(&sems[task_no]);
		PRINT_LOCK; 
		DISPLAY(strs[task_no]);
		PRINT_UNLOCK;
		if (task_no == (NUM_TASKS - 1)) {
			PRINT_LOCK; 
			DISPLAY("\n");
			PRINT_UNLOCK;
		}
		sem_post_rt(&sems[(task_no + 1) % NUM_TASKS]);
	}
	sem_post_rt(&sync_sem);
	sem_wait_rt(&prio_sem);
	PRINT_LOCK; 
	DISPLAY(strs[task_no]);
	PRINT_UNLOCK;
	rt_sleep(nano2count(1000000000LL));
	nanos2timespec(rt_get_time_ns() + (task_no + 1)*1000000000LL, &t);
	sem_timedwait_rt(&prio_sem, &t);
	PRINT_LOCK;
	DISPLAY("sem timeout, task %d, %s\n", task_no, strs[task_no]);
	PRINT_UNLOCK;
	sem_post_rt(&sync_sem);

	/* message queue stuff */
	mq_receive(mq_in, buf, sizeof(buf), &i);
	PRINT_LOCK; 
	DISPLAY("\nreceived by task %d ", task_no);
	DISPLAY(buf);
	PRINT_UNLOCK;
	mq_send(mq_out, strs[task_no], strlen(strs[task_no]) + 1, 1);

	/* test receive timeout */
	sem_wait_rt(&sync_sem);
	nanos2timespec(rt_get_time_ns() + (task_no + 1)*1000000000LL, &t);
	if (mq_timedreceive(mq_in, buf, sizeof(buf), &i, &t) == -ETIMEDOUT) {
		PRINT_LOCK;
		DISPLAY("\nmbx timeout, task %d, %s\n", task_no, strs[task_no]);
		PRINT_UNLOCK;
	}
	mq_close(mq_in);
	mq_close(mq_out);
	PRINT_LOCK;
	DISPLAY("task %d complete\n", task_no);
	PRINT_UNLOCK;
	pthread_exit_rt(0);
}

/*
 * initialization task
 */
static void *start_task_code(void *arg)
{
	int i, k;
	mqd_t mq_in, mq_out;
	char buf[MAX_MSG_SIZE];

	rt_task_init_schmod(nam2num("STSKOD"), NUM_TASKS + 10, 0, 0, SCHED_FIFO, 0xF);
	pthread_mutex_init_rt(&print_mtx, NULL);
	mq_in  = mq_open("mq_in", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &mqattrs);
	mq_out = mq_open("mq_out", O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &mqattrs);
	for (i = 0; i < NUM_TASKS; ++i) {
		sem_init_rt(&sems[i], SEM_BINARY, 0);
		pthread_create_rt(&thread[i], NULL, (void *)task_code, (void *)i);
	}	
	/* create the sync semaphore */
	sem_init_rt(&sync_sem, 0, 0);
	/* create the priority-test semaphore */
	sem_init_rt(&prio_sem, SEM_BINARY, 0);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_barrier_wait_rt(&barrier);
	/* pass the semaphore to the first task */
	sem_post_rt(&sems[0]);
	/* wait for each task to send the sync semaphore */
	for (i = 0; i < NUM_TASKS; ++i) {
		sem_wait_rt(&sync_sem);
	}
	PRINT_LOCK; 
	DISPLAY(sync_str);
	PRINT_UNLOCK;
	/* post the priority-test semaphore -- the tasks should then run */
	/* in priority order */
	for (i = 0; i < NUM_TASKS; ++i) {
		sem_post_rt(&prio_sem);
	}
	PRINT_LOCK; 
	DISPLAY("\n");
	PRINT_UNLOCK;
	for (i = 0; i < NUM_TASKS; ++i) {
		sem_wait_rt(&sync_sem);
	}
	PRINT_LOCK; 
	DISPLAY(sync_str);

	/* now, test message queues */
	DISPLAY("testing message queues\n");
	PRINT_UNLOCK;
	for (i = 0; i < NUM_TASKS; ++i) {
		mq_send(mq_in, strs[i], strlen(strs[i]) + 1, 1);
	}
	for (i = 0; i < NUM_TASKS; ++i) {
		mq_receive(mq_out, buf, sizeof(buf), &k);
		PRINT_LOCK; 
		DISPLAY("\nreceived from mq_out: %s", buf);
		PRINT_UNLOCK;
	}
	for (i = 0; i < NUM_TASKS; ++i) {
		sem_post_rt(&sync_sem);
	}
	PRINT_LOCK; 
	DISPLAY("\n");
	PRINT_UNLOCK;

	/* nothing more for this task to do */
	for (i = 0; i < NUM_TASKS; ++i) {
		pthread_join_rt(thread[i], NULL);
		sem_destroy_rt(&sems[i]);
	}
	mq_close(mq_in);
	mq_close(mq_out);
	mq_unlink("mq_in");
	mq_unlink("mq_out");
	sem_destroy_rt(&sync_sem);
	pthread_mutex_destroy_rt(&print_mtx);
	DISPLAY("\ninitialization task complete\n");
	pthread_exit_rt(0);
}

int main(void)
{
	printf("\n");
	rt_task_init_schmod(nam2num("MAIN"), NUM_TASKS + 20, 0, 0, SCHED_FIFO, 0xF);
	rt_set_oneshot_mode();
	start_rt_timer(0);
	pthread_barrier_init_rt(&barrier, 0, NUM_TASKS + 2);
	pthread_create_rt(&start_thread, NULL, start_task_code, NULL);
	pthread_barrier_wait_rt(&barrier);
	pthread_join_rt(start_thread, NULL);
	pthread_barrier_destroy_rt(&barrier);
	stop_rt_timer();
	printf("\n");
	pthread_exit_rt(0);
}
