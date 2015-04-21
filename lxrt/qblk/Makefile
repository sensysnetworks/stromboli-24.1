
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -llxrt

A_TARG := test_hrt test_proxy test_sft

MI_OBJS := rt_process.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/qblk
clean:
	make -C ../.. subclean SUBDIRS=lxrt/qblk
endif

