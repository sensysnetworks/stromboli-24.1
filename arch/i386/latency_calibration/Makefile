MI_OBJS := rt_cal.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../../.. modules SUBDIRS=arch/i386/latency_calibration
clean:
	make -C ../../.. subclean SUBDIRS=arch/i386/latency_calibration
endif

cal:
	../../../scripts/rt_modprobe rt_cal.o

stop:
	../../../scripts/rt_rmmod rt_cal
