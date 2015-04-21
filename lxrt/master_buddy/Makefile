
EXTRA_USER_CFLAGS := -I ..

A_TARG  = master_proc buddy_proc

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/master_buddy
clean:
	make -C ../.. subclean SUBDIRS=lxrt/master_buddy
endif

