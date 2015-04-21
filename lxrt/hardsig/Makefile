
EXTRA_USER_CFLAGS := -I ..

A_TARG  = rt_task spv

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/hardsig
clean:
	make -C ../.. subclean SUBDIRS=lxrt/hardsig
endif

