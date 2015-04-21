EXTRA_USER_CFLAGS := -I. -I../lxrt

ifneq ($(CONFIG_RTAI_FIFOS_LXRT),)
  M_OBJS += rtai_fifos_lxrt.o
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=fifos_lxrt
clean:
	make -C .. subclean SUBDIRS=fifos_lxrt
endif



