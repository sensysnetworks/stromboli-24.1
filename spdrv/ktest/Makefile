EXTRA_CFLAGS := -I. -I..

MI_OBJS += hello_world.o
MI_OBJS += hello_world2.o
MI_OBJS += testcom.o
MI_OBJS += test-tx.o test-rx.o test-rx-thrs.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=spdrv/ktest
clean:
	make -C ../.. subclean SUBDIRS=spdrv/ktest
endif
