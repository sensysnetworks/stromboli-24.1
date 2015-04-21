
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -llxrt -lpthread

A_TARG := rt_process1 rt_process2

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/switches
clean:
	make -C ../.. subclean SUBDIRS=lxrt/switches
endif


