ifneq ($(CONFIG_RTAI_TBX),)
  ALL_SUB_DIRS := tbx-test
  MOD_SUB_DIRS := tbx-test
  SUB_DIRS     := tbx-test
  M_OBJS       := rtai_tbx.o rtai_tbx_lxrt.o rtai_pqueue.o rtai_pqueue_lxrt.o
  MI_OBJS      := 
  MIX_OBJS     := 
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=tbx
clean:
	make -C .. subclean SUBDIRS=tbx
endif

rtai_tbx.c:	tbx.c
	ln -sf $< $@
