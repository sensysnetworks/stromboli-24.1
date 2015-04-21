ifneq ($(CONFIG_RTAI_USI),)
  MX_OBJS += rtai_usi.o
  ALL_SUB_DIRS := test
  MOD_SUB_DIRS := test
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=usi
clean:
	make -C .. subclean SUBDIRS=usi
endif
