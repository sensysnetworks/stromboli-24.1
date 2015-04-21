

ifeq ($(CONFIG_RTAI_DYN_MM),m)
  MX_OBJS := rt_mem_mgr.o
endif
ifneq ($(CONFIG_RTAI_DYN_MM),n)
  ifneq ($(CONFIG_RTAI_TESTS),)
    MI_OBJS := rt_mem_test.o rt_mem_test2.o rt_mem_test3.o
  endif
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=rt_mem_mgr
clean:
	make -C .. subclean SUBDIRS=rt_mem_mgr
endif


