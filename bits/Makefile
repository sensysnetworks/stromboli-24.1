ifneq ($(CONFIG_RTAI_BITS),)
ALL_SUB_DIRS := kbitstest ubitstest
MOD_SUB_DIRS := kbitstest ubitstest
M_OBJS += rtai_bits.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=bits
clean:
	make -C .. subclean SUBDIRS=bits
endif
