
EXTRA_CFLAGS := -I .. -DLXRT_MODULE -DLINUX_SIGNAL_HANDLER

M_OBJS := rtai_aio.o
MI_OBJS += rtai_aiotest.o tst-rtai_syscall.o

aio_objs := 	aio.o aio_cancel.o aio_cmd.o aio_misc.o \
		aio_notify.o aio_suspend.o lio_listio.o \
		rtai_syscall.o

MIX_OBJS := aio.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/aio
clean:
	make -C ../.. subclean SUBDIRS=lxrt/aio
endif

rtai_aio.o:	$(aio_objs)
	$(LD) -r -o $@ $(aio_objs)


rtai_aiotest.c: tst-aio.c
	ln -sf $< $@

