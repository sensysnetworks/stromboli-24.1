
EXTRA_USER_LIBS += -lpthread
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)
  A_TARG += player speaker
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/usound
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/usound
endif

