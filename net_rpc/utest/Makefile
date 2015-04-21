
EXTRA_USER_LIBS += -lpthread
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)
  A_TARG += comnode start_task task_code
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/utest
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/utest
endif

