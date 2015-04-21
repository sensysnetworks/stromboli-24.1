VERSION = 24
PATCHLEVEL = 1
SUBLEVEL = 11
EXTRAVERSION =

PROJECT = RTAI
project = rtai

ifeq ($(wildcard .buildvars), .buildvars)
include .buildvars
ifeq ($(wildcard $(LINUXDIR)/.config), $(LINUXDIR)/.config)
include $(LINUXDIR)/.config
endif
endif

ifeq ($(CONFIG_ARM), y)
SUBDIRS := rtaidir rt_mem_mgr shmem smpscheduler mupscheduler upscheduler \
	   fifos latency_calibration examples tasklets mups_examples \
	   tbx posix watchdog testing trace rt_com libm libsf
else
ifeq ($(CONFIG_PPC), y)
SUBDIRS := rtaidir rt_mem_mgr shmem smpscheduler mupscheduler upscheduler \
	   fifos latency_calibration examples tasklets mups_examples \
	   tbx posix watchdog testing trace rt_com libm leds rtnet libsf spdrv
else
ifeq ($(CONFIG_ETRAX100LX_V2), y)
SUBDIRS := rtaidir rt_mem_mgr upscheduler \
          fifos latency_calibration examples tasklets \
          tbx posix watchdog testing trace rt_com libm leds rtnet libsf
EXTRA_USER_CFLAGS := -mlinux
EXTRA_USER_LDFLAGS := -mlinux
else
SUBDIRS := rtaidir rt_mem_mgr shmem smpscheduler mupscheduler upscheduler \
	   fifos latency_calibration examples lxrt tasklets mups_examples \
	   tbx posix watchdog testing trace rt_com_lxrt rt_com libm net_rpc \
	   leds rtnet bits libsf rtai_cpp fifos_lxrt newlxrt usi spdrv \
	   usposix comedi_lxrt
endif
endif
endif

include ./Makefile.modbuild

proj-deps:

prefix := $(DESTDIR)/usr
rt_prefix := $(DESTDIR)/usr/realtime

install_rt_devel:
	install -d $(prefix)/bin
	install -m 755 scripts/realtime-config $(prefix)/bin
	install -d $(rt_prefix)/include/rtai
	(cd include;install -m 644 *.h $(rt_prefix)/include/rtai)
	install -d $(rt_prefix)/include/rtai/asm
	(cd include/asm;install -m 644 *.h $(rt_prefix)/include/rtai/asm)
	# Fixups, because header files are screwed up
	(cd $(rt_prefix)/include/;ln -s rtai/*.h .)
	install -d $(rt_prefix)/include/asm
	(cd $(rt_prefix)/include/asm;ln -s ../rtai/asm/*.h .)

libversion=0.0.1
install_runtime:
	install -d $(prefix)/bin
	install -m 755 scripts/rt_modprobe $(prefix)/bin
	install -m 755 scripts/rt_rmmod $(prefix)/bin
	install -m 755 testing/test_shmem_1 $(prefix)/bin
#	install -m 755 testing/test_fifos_1 $(prefix)/bin
ifdef CONFIG_RTAI_TESTS
	install -m 755 testing/fifos/regression $(prefix)/bin
	install -m 755 testing/fifos/regression2 $(prefix)/bin
	install -m 755 testing/fifos/regression3 $(prefix)/bin
endif
ifdef CONFIG_RTAI_LXRT
	install -m 644 lxrt/lib/liblxrt.so.0.0.1 $(prefix)/lib
	(cd $(prefix)/lib;ln -s liblxrt.so.$(libversion) liblxrt.so.0)
	(cd $(prefix)/lib;ln -s liblxrt.so.$(libversion) liblxrt.so)
	install -m 644 lxrt/lib/liblxrt.a $(prefix)/lib
endif

dev:
	@if [ -c $(INSTALL_MOD_PATH)/dev/rtai_shm ]; then \
		echo "Not creating $(INSTALL_MOD_PATH)/dev/rtai_shm: file exists"; \
	else mknod -m 666 $(INSTALL_MOD_PATH)/dev/rtai_shm c 10 254; fi
	@for n in 0 1 2 3 4 5 6 7 8 9; do \
		f="$(INSTALL_MOD_PATH)/dev/rtf$$n"; \
		if [ -c $$f ]; then \
			echo "Not creating $$f: file exists"; \
		else mknod -m 666 $$f c 150 $$n; \
		fi; \
	done
