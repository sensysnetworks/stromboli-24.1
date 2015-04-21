
EXTRA_USER_CFLAGS := -I ..

A_TARG := rt_process

MI_OBJS := handler.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/resumefromintr
clean:
	make -C ../.. subclean SUBDIRS=lxrt/resumefromintr
endif

