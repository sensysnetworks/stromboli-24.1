
ALL_SUB_DIRS := 
MOD_SUB_DIRS := 
SUB_DIRS     := 

ifneq ($(CONFIG_RTAI_TRACE),)
M_OBJS       := rtai_trace.o
endif
MI_OBJS      := 
MIX_OBJS     := 



ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=trace
clean:
	make -C .. subclean SUBDIRS=trace
endif




