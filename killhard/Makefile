EXTRA_USER_CFLAGS := -I ..  -I ../lxrt

A_TARG = killh

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=killhard
clean:
	make -C .. subclean SUBDIRS=killhard
endif
