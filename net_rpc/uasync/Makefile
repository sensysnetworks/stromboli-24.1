
EXTRA_USER_LIBS += -lpthread -llxrt
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)
  A_TARG += sender receiver
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/uasync
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/uasync
endif
