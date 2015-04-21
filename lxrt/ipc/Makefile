
EXTRA_USER_CFLAGS := -I .. -I../leds
EXTRA_USER_LIBS := ../leds/libleds.a -llxrt

A_TARG := clt srv

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/ipc
clean:
	make -C ../.. subclean SUBDIRS=lxrt/ipc
endif

