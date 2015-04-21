
EXTRA_CFLAGS := -I.

MI_OBJS	+= ex_prinh1.o ex_prinh2.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/prioinher
clean:
	make -C ../.. clean SUBDIRS=examples/prioinher
endif

ex_prinh1.c:	rt_process1.c
	ln -sf $< $@

ex_prinh2.c:	rt_process2.c
	ln -sf $< $@

