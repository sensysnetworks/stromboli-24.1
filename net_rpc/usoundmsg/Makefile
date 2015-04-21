
EXTRA_USER_LIBS += -lpthread
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)
  A_TARG += player speaker1 speaker2 speaker3
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/usoundmsg
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/usoundmsg
endif
