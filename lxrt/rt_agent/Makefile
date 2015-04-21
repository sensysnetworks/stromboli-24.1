
EXTRA_USER_CFLAGS := -I ..

MI_OBJS := rt_agent.o

A_TARG := sender receiver

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/rt_agent
clean:
	make -C ../.. subclean SUBDIRS=lxrt/rt_agent
endif

