
EXTRA_USER_CFLAGS := -I .. -D_REENTRANT
EXTRA_USER_LIBS := -lpthread

A_TARG := rt_process

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/threads
clean:
	make -C ../.. subclean SUBDIRS=lxrt/threads
endif

