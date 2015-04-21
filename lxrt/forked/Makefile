
EXTRA_USER_CFLAGS := -I ..

A_TARG  = rt_process

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/forked
clean:
	make -C ../.. subclean SUBDIRS=lxrt/forked
endif

