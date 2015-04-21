EXTRA_USER_LIBS += -lpthread -llxrt
EXTRA_USER_CFLAGS := -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)

  M_OBJS += krtnet.o

  ifneq ($(CONFIG_RTAI_LXRT),)
    A_TARG += urtnet
  endif

endif 

EXTRA_CFLAGS += -DURTNET=$(TOPDIR)/net_rpc/rtnet/urtnet

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/rtnet
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/rtnet
endif


