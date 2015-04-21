EXTRA_USER_LIBS += -lpthread -lm
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)
  A_TARG += param scope mbplot
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/scicos
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/scicos
endif
