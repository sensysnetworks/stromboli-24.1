EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -lpthread

A_TARG := rt_process rt_tasklet

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=usi/test
clean:
	make -C ../.. subclean SUBDIRS=usi/test
endif
