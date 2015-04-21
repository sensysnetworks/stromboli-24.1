
#ifndef _RTAI_SIGNAL_H_
#define _RTAI_SIGNAL_H_

extern int rt_set_linux_signal_handler(RT_TASK *task, void (*handler)(int sig));
extern int rt_get_linux_signal(RT_TASK *task);
extern int rt_get_errno(RT_TASK *task);

extern void rt_init_linux_signal_handler(void);
extern void rt_remove_linux_signal_handler(void);
extern int rt_lxrt_fork(struct pt_regs *regs, int is_a_clone);

#endif // _RTAI_SIGNAL_H_
