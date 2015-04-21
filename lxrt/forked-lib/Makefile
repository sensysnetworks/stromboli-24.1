
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -llxrt

A_TARG  = rt_process

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/forked-lib
clean:
	make -C ../.. subclean SUBDIRS=lxrt/forked-lib
endif

