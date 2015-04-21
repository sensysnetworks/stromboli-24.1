EXTRA_USER_CFLAGS := -I. -I.. -I../../lxrt -I../spdrv

A_TARG += hello_world2_lxrt hello_world_lxrt testcomlxrt

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=spdrv/utest
clean:
	make -C ../.. subclean SUBDIRS=spdrv/utest
endif
