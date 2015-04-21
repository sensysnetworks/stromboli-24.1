ifeq ($(CONFIG_RTAI_WATCHDOG),m)
ALL_SUB_DIRS :=
MOD_SUB_DIRS := 
SUB_DIRS     := 

M_OBJS       := rtai_watchdog.o
MI_OBJS      := 
MIX_OBJS     := 
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=watchdog
clean:
	make -C .. subclean SUBDIRS=watchdog
endif
