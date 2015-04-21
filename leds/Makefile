
MX_OBJS	+= rtai_leds.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=leds
clean:
	make -C .. subclean SUBDIRS=leds
endif



