
MI_OBJS	:= ex_condtest.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/condtest
clean:
	make -C ../.. subclean SUBDIRS=examples/condtest
endif

ex_condtest.c:	rt_process.c
	ln -sf $< $@

