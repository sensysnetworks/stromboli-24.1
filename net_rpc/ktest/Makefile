ifeq ($(CONFIG_RTAI_NETRPC),m)
  MI_OBJS += comnode.o start_task.o task_code.o
endif

ifdef TOPDIR
  include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=net_rpc/ktest
clean:
	make -C ../.. subclean SUBDIRS=net_rpc/ktest
endif

