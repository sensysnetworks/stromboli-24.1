ALL_SUB_DIRS  := test latency_calibration mbx simple

ifdef CONFIG_RTAI_EXAMPLES
  EXAMPLE_DIRS  := test latency_calibration mbx simple 
endif

MOD_SUB_DIRS  := $(EXAMPLE_DIRS)
APP_SUB_DIRS  := $(EXAMPLE_DIRS)
SUB_DIRS      := $(EXAMPLE_DIRS)

EXTRA_CFLAGS := 


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=rtai_cpp/examples
clean:
	make -C ../.. subclean SUBDIRS=rtai_cpp/examples
endif


