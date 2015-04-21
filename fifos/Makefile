

ifeq ($(CONFIG_RTAI_FIFOS),m)
  MX_OBJS += rtai_fifos.o
endif



ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=fifos
clean:
	make -C .. subclean SUBDIRS=fifos
endif


