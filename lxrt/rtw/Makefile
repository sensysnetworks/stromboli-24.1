
ALL_SUB_DIRS := drivers
MOD_SUB_DIRS := drivers

EXTRA_CFLAGS := -I.

EXTRA_USER_CFLAGS := -DDEBUG -DDBGPRT

MI_OBJS := rt_ni_e_timer.o rtc.o sysAuxClk.o

A_TARG	:= check rtai_main

EXTRA_USER_LIBS := -lpthread

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/rtw
clean:
	make -C ../.. subclean SUBDIRS=lxrt/rtw
endif
