
#EXTRA_CFLAGS := -I. -I../lxrt
EXTRA_USER_CFLAGS := -I. -I../lxrt -I../rt_com

ifneq ($(CONFIG_RTAI_RTCOM_LXRT),)
  M_OBJS += rt_com_lxrt.o

  A_TARG += hello_world2_lxrt hello_world_lxrt testcomlxrt
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=rt_com_lxrt
clean:
	make -C .. subclean SUBDIRS=rt_com_lxrt
endif



