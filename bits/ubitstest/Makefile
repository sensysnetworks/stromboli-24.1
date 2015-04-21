
EXTRA_USER_LIBS += -lpthread
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

A_TARG += rt_process

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=bits/ubitstest
clean:
	make -C ../.. subclean SUBDIRS=bits/ubitstest
endif

