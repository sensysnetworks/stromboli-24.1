
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxrt

ifeq ($(CONFIG_RTAI_NETRPC),m)
  MI_OBJS += handler.o
  A_TARG += rt_process
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/uresumefromintr
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/uresumefromintr
endif

