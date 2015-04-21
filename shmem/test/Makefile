ifneq ($(CONFIG_RTAI_TESTS),)
  MI_OBJS += kmalloc.o

  A_TARG += itest
  A_TARG += ctest
  A_TARG += ftest
endif

EXTRA_USER_CFLAGS += -DDEBUG


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=shmem/test
clean:
	make -C ../.. subclean SUBDIRS=shmem/test
endif
