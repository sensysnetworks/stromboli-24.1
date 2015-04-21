
ALL_SUB_DIRS := test
MOD_SUB_DIRS := test

ifneq ($(CONFIG_RTAI_RTCOM),)
  MX_OBJS += rt_com.o
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=rt_com
clean:
	make -C .. subclean SUBDIRS=rt_com
endif



