
EXTRA_USER_CFLAGS := -I ..

A_TARG  = user_hrt

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/one
clean:
	make -C ../.. subclean SUBDIRS=lxrt/one
endif

