
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -llxrt

A_TARG := displog sendlog

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/testmbx_wp
clean:
	make -C ../.. subclean SUBDIRS=lxrt/testmbx_wp
endif

