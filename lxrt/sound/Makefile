
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -lpthread

A_TARG  = rt_process

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/sound
clean:
	make -C ../.. subclean SUBDIRS=lxrt/sound
endif

