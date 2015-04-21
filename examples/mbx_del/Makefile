
EXTRA_CFLAGS := -I.

MI_OBJS	+= ex_mbx_del.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/mbx_del
clean:
	make -C ../.. subclean SUBDIRS=examples/mbx_del
endif

ex_mbx_del.c: rt_process.c
	ln -sf $< $@
