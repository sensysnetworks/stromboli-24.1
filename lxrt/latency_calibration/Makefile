
EXTRA_USER_CFLAGS := -I ..

A_TARG := rt_process check

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/latency_calibration
clean:
	make -C ../.. subclean SUBDIRS=lxrt/latency_calibration
endif

