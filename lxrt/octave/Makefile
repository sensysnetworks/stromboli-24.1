
MKOCTFILE = mkoctfile

octinfo = $(shell ./octinfo.sh $(MKOCTFILE))
verparts = $(subst ., ,$(word 1,$(octinfo)))
ver = $(word 1,$(verparts))$(word 2,$(verparts))

%.oct: %.cc ; $(MKOCTFILE) -DHAVE_OCTAVE_$(ver) -I../../include $< -L../lib -llxrt

MAIN  = rtai_lxrt.oct
PROGS = rt_task_init.oct rt_sleep.oct lock_all.oct init_linux_scheduler.oct \
		rt_task_delete.oct start_rt_timer.oct rt_set_oneshot_mode.oct \
		nam2num.oct rtai_print_to_screen.oct rt_make_soft_real_time.oct \
		rt_make_hard_real_time.oct rt_task_resume.oct rt_task_suspend.oct \
		rt_get_time.oct

all: $(MAIN) $(PROGS)

$(PROGS):
	ln -sf $(MAIN) $@

clean: 
	$(RM) core *.S *.log *.o $(PROGS) octave-core $(MAIN)  
