
EXTRA_CFLAGS := -I.

ifneq ($(CONFIG_RTAI_SCHED_MUP),)
  M_OBJS	+= rtai_sched_mup.o
endif

sched_objs := rtai_sched.o

ifeq ($(CONFIG_PPC),y)
sched_objs += up_task_sw.o
endif
ifeq ($(CONFIG_RTAI_DYN_MM),y)
sched_objs += rt_mem_mgr.o
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=mupscheduler
clean:
	make -C .. subclean SUBDIRS=mupscheduler
endif

rtai_sched_mup.o: $(sched_objs)
	$(LD) -r -o $@ $(sched_objs)

# The rest of these rules are hacks because the files are not in
# the right place.  Please do not use this in new code.

rtai_sched.c: rtai_sched.c.ml
	ln -sf $< $@

up_task_sw.o: up_task_sw.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $@

up_task_sw.S: ../include/asm/up_task_sw.S
	ln -sf $< $@

rt_mem_mgr.c: ../rt_mem_mgr/rt_mem_mgr.c
	ln -sf $< $@
