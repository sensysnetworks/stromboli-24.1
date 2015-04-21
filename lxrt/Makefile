
ignore_subdirs := octave perl

subdirs := lib leds forked forked-lib hardsig ipc latency_calibration \
	many master_buddy msg_clock one pressa qblk fifosound \
	resumefromintr rt_agent sem_clock sound switches testmbx_wp \
	threads traps two unix sched_ext soundmsg aio C++

ifdef CONFIG_RTAI_LXRT_OCTAVE
	subdirs += octave
endif

ifdef CONFIG_RTAI_LXRT_RTW
	subdirs += rtw
endif

# list of examples that require shared memory
ifndef CONFIG_RTAI_SHMEM
	disable += rt_agent unix
endif

# list of examples that require FIFOs
ifndef CONFIG_RTAI_FIFOS
	disable += fifosound
endif

ALL_SUB_DIRS := $(subdirs) rtw
ifdef CONFIG_RTAI_LXRT_EXAMPLES
	MOD_SUB_DIRS := $(filter-out $(disable), $(subdirs))
	SUB_DIRS     := $(filter-out $(disable), $(subdirs))
else
	MOD_SUB_DIRS := lib
	SUB_DIRS     := lib
endif

EXTRA_CFLAGS := -I. -DLXRT_MODULE -DLINUX_SIGNAL_HANDLER

rtai_lxrt-objs := lxrt.o hash.o msg.o tid.o proxies.o names.o qblk.o \
	usrf.o informed.o registry.o lxrt_table.o signal.o

ifneq ($(CONFIG_RTAI_LXRT),)
	M_OBJS += rtai_lxrt_old.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=lxrt
clean:
	make -C .. subclean SUBDIRS=lxrt
endif

ifeq ($(CONFIG_RTAI_LXRT_KTRD),y)
# Paolo wants it so till it is not really finished
lxrt.c: dummy
	diff lxrt.c.ktrd lxrt.c > .dfile; if [ -s .dfile ]; then cp -f lxrt.c.ktrd lxrt.c; fi
endif

ifeq ($(CONFIG_RTAI_LXRT_NOKTRD),y)
# Paolo wants it so till it is not really finished
lxrt.c: dummy
	diff lxrt.c.noktrd lxrt.c > .dfile; if [ -s .dfile ]; then cp -f lxrt.c.noktrd lxrt.c; fi
endif

rtai_lxrt_old.o: $(rtai_lxrt-objs)
	$(LD) -r -o $@ $(rtai_lxrt-objs)
