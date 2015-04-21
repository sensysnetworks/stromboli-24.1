
EXTRA_USER_CFLAGS := -I ..

M_OBJS := rtai_sched_ext.o

MI_OBJS := rt_process.o sched_ext.o 

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/sched_ext
clean:
	make -C ../.. subclean SUBDIRS=lxrt/sched_ext
endif

OBJS = ../hash.o ../msg.o ../tid.o ../proxies.o ../names.o ../registry.o

rtai_sched_ext.o: sched_ext.o $(OBJS)
	$(LD) -r -o $@ sched_ext.o $(OBJS)

