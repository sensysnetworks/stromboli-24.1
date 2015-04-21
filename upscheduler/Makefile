
EXTRA_CFLAGS := -I.

ifneq ($(CONFIG_RTAI_SCHED_UP),)
  M_OBJS += rtai_sched_up.o
endif
  M_OBJS += rtai_sched_up.o

sched_objs := rtai_sched.o
ifeq ($(CONFIG_PPC),y)
sched_objs += up_task_sw.o
endif
ifeq ($(CONFIG_MIPS),y)
sched_objs += up_task_sw.o
endif
ifeq ($(CONFIG_ARM),y)
sched_objs += up_task_sw.o
endif
ifeq ($(CONFIG_ETRAX100LX),y)
sched_objs += up_task_sw.o
endif
ifeq ($(CONFIG_ETRAX100LX_V2),y)
sched_objs += up_task_sw.o
endif
ifeq ($(CONFIG_RTAI_DYN_MM),y)
sched_objs += rt_mem_mgr.o
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=upscheduler
clean:
	make -C .. subclean SUBDIRS=upscheduler
endif

rtai_sched.c: rtai_sched.c.ml
	ln -sf $< $@

up_task_sw.S: ../include/asm/up_task_sw.S
	ln -sf $< $@

up_task_sw.o: up_task_sw.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $@

rt_mem_mgr.c: ../rt_mem_mgr/rt_mem_mgr.c
	ln -sf $< $@

rtai_sched_up.o: $(sched_objs)
	$(LD) -r -o $@ $(sched_objs)

