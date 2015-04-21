ALL_SUB_DIRS := ktest utest

ifeq ($(CONFIG_RTAI_SPDRV),m)
	ifeq ($(CONFIG_8xx),y)
		MX_OBJS += rtai_spdrv_mpc8xx.o
	else
		MX_OBJS += rtai_spdrv.o
	endif
	SUB_DIRS := ktest
	MOD_SUB_DIRS := ktest
	ifeq ($(CONFIG_RTAI_LXRT),m)
		MX_OBJS += rtai_spdrv_lxrt.o
		SUB_DIRS += utest
		MOD_SUB_DIRS += utest
	endif
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=spdrv
clean:
	make -C .. subclean SUBDIRS=spdrv
endif
