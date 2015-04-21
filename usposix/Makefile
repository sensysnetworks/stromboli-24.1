EXTRA_USER_CFLAGS := -D_XOPEN_SOURCE=600 -I$(LINUXDIR)/include
EXTRA_USER_LIBS := -lpthread

A_TARG  = condtest mqtest jepplin rwlocks

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../ modules SUBDIRS=usposix
clean:
	make -C ../ subclean SUBDIRS=usposix
endif
