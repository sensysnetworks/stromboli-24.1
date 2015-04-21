#! /usr/local/bin/octave -qfH

system("sync");
lock_all( 128*1024, 1024*1024);
SCHED_FIFO = 1;
init_linux_scheduler( SCHED_FIFO, 99);

#
# This is going to be an FPU intensive example...
#
function y=foo(x);
	y=sin(x)*16+sin(2*x)*8+sin(4*x)*4+sin(8*x)*2+sin(16*x);
endfunction;

x=[foo(0:pi/1024:pi*2)];

rt_set_oneshot_mode();
start_rt_timer(0);

task = rt_task_init( nam2num("OCTAVE"), 0, 0, 0);
rt_printk("RTAI task %X starts...\n", task);

ignore_function_time_stamp = "all";
auto_unload_dot_oct_files = 0;
diary off;

rt_printk("ignore_function_time_stamp %s\n", ignore_function_time_stamp);
rt_printk("auto_unload_dot_oct_files %d\n", auto_unload_dot_oct_files);

# Force pre-loading.
s1 = computer();
rt_sleep(10000);
t1 = columns(x);
# The first call of rt_make_soft_real_time()
# does nothing as far as LXRT is concerned 
# but *does* force Octave to preload the function.
t1 = rt_get_time();
rt_make_soft_real_time(); 

if (strcmp(argv(1,:), "HRT"))
    rt_printk("HRT mode\n");
	rt_make_hard_real_time();
endif
for i = 1:10;
	rt_sleep(100000000);
	rt_printk("rt_sleep(%d), foo(%d) = %f\n", i, i, foo(i));
	t1 = rt_get_time();
	data = fft(x); # What the heck, let's do a fast fourrier transform.
	t2 = rt_get_time();
    rt_printk("CPU cycles for %d points FFT %d on %s\n", columns(x), t2-t1, computer());
endfor;
rt_make_soft_real_time();

rt_task_delete(task);
rt_printk("Done.\n");
