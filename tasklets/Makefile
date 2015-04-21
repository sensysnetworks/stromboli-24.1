
ALL_SUB_DIRS := tests
SUB_DIRS     :=
MOD_SUB_DIRS :=

ifneq ($(CONFIG_RTAI_TASKLETS),)
  SUB_DIRS     += $(ALL_SUB_DIRS)
  MOD_SUB_DIRS += $(ALL_SUB_DIRS)
  M_OBJS := rtai_tasklets.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=tasklets
clean:
	make -C .. subclean SUBDIRS=tasklets
endif

rtai_tasklets.c: tasklets.c
	ln -sf $< $@

