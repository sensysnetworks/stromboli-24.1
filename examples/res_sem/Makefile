
EXTRA_CFLAGS := -I.

MI_OBJS += ex_res_sem.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/res_sem
clean:
	make -C ../.. subclean SUBDIRS=examples/res_sem
endif

ex_res_sem.c:	rt_process.c
	ln -sf $< $@

