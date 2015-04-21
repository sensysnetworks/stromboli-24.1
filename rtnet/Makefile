
subdirs := tools

ALL_SUB_DIRS := $(subdirs)
MOD_SUB_DIRS := $(subdirs)
SUB_DIRS     := $(subdirs)

ifeq ($(CONFIG_RTAI_RTNET),m)
  M_OBJS += rtnet.o
  MIX_OBJS += rtnet_syms.o
  rtnet-objs += rtnet_syms.o rtnet_dev.o socket.o dev.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=rtnet
clean:
	make -C .. subclean SUBDIRS=rtnet
endif


rtnet.o: $(rtnet-objs)
	$(LD) -r -o $@ $(rtnet-objs)

