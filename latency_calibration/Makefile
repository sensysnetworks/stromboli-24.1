
ifdef CONFIG_RTAI_FIFOS
MI_OBJS	:= latency_calibrate.o
A_TARG := check
else
MI_OBJS	:=
A_TARG :=
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=latency_calibration
clean:
	make -C .. subclean SUBDIRS=latency_calibration
endif

latency_calibrate.c: rt_process.c
	ln -sf $< $@
