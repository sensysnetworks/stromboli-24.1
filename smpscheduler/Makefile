
CFLAGS_rtai_sched.o := -D__USE_APIC__

ifndef ($(CONFIG_RTAI_SCHED_SMP),)
  ifeq ($(CONFIG_X86),y)
    M_OBJS += rtai_sched_smp_8254.o
    ifeq ($(CONFIG_SMP),y)
    M_OBJS += rtai_sched_smp.o
    endif
  endif
  ifdef ($(CONFIG_PPC),y)
    M_OBJS += rtai_sched_smp.o
  endif
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
	make -C .. modules SUBDIRS=smpscheduler
clean:
	make -C .. subclean SUBDIRS=smpscheduler
endif


rtai_sched_smp.o: $(sched_objs)
	$(LD) -r -o $@ $(sched_objs)

# The rest of these rules are hacks because the files are not in
# the right place.  Please do not use this in new code.

rtai_sched.c: rtai_sched.c.ml
	ln -sf $< $@

rtai_sched_8254.c: rtai_sched.c.ml
	ln -sf $< $@

up_task_sw.o: up_task_sw.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $@

up_task_sw.S: ../include/asm/up_task_sw.S
	ln -sf $< $@

rt_mem_mgr.c: ../rt_mem_mgr/rt_mem_mgr.c
	ln -sf $< $@

rtai_sched_smp_8254.o: $(subst rtai_sched.o,rtai_sched_8254.o, $(sched_objs))
	$(LD) -r -o $@ $(subst rtai_sched.o,rtai_sched_8254.o, $(sched_objs))

