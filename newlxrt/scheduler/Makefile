EXTRA_CFLAGS := -I.

M_OBJS += rtai_sched_newlxrt.o

sched_objs := rtai_sched.o

ifeq ($(CONFIG_RTAI_DYN_MM),y)
sched_objs += rt_mem_mgr.o
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=newlxrt/scheduler
clean:
	make -C ../.. subclean SUBDIRS=newlxrt/scheduler
endif

rtai_sched_newlxrt.o: $(sched_objs)
	$(LD) -r -o $@ $(sched_objs)

# The rest of these rules are hacks because the files are not in
# the right place.  Please do not use this in new code.

ifeq ($(CONFIG_RTAI_NEWLXRT_NOBUDDY),y)
rtai_sched.c: dummy
	diff rtai_sched.c.nobuddy rtai_sched.c > .dfile; if [ -s .dfile ]; then cp -f rtai_sched.c.nobuddy rtai_sched.c; fi
endif

ifeq ($(CONFIG_RTAI_NEWLXRT_BUDDY),y)
rtai_sched.c: dummy
	diff rtai_sched.c.buddy rtai_sched.c > .dfile; if [ -s .dfile ]; then cp -f rtai_sched.c.buddy rtai_sched.c; fi
endif

rt_mem_mgr.c: ../../rt_mem_mgr/rt_mem_mgr.c
	ln -sf $< $@
