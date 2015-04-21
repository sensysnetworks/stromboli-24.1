
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -lpthread

A_TARG  = rt_process

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/fifosound
clean:
	make -C ../.. subclean SUBDIRS=lxrt/fifosound
endif

