
EXTRA_USER_CFLAGS := -I .. -Wno-format

A_TARG := unix_lxrt test

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/unix
clean:
	make -C ../.. subclean SUBDIRS=lxrt/unix
endif

