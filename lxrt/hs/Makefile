
EXTRA_USER_CFLAGS := -I ..

A_TARG := test

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/hs
clean:
	make -C ../.. subclean SUBDIRS=lxrt/hs
endif

