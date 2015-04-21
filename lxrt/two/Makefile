
EXTRA_USER_CFLAGS := -I ..

A_TARG  = testa testb

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/two
clean:
	make -C ../.. subclean SUBDIRS=lxrt/two
endif

