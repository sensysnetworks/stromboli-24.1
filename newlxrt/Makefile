subdirs := scheduler

ALL_SUB_DIRS := $(subdirs)
MOD_SUB_DIRS := 
SUB_DIRS     := 

ifdef CONFIG_RTAI_NEWLXRT
SUB_DIRS     += $(subdirs)
MOD_SUB_DIRS += $(subdirs)
endif

newlxrt-objs := newlxrt.o tabnewlxrt.o msgnewlxrt.o infnewlxrt.o proxies.o registry.o 

ifneq ($(CONFIG_RTAI_NEWLXRT),)
  M_OBJS += rtai_lxrt_new.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=newlxrt
clean:
	make -C .. subclean SUBDIRS=newlxrt
endif

rtai_lxrt_new.o: $(newlxrt-objs)
	$(LD) -r -o $@ $(newlxrt-objs)
