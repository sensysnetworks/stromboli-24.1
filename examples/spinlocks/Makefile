MI_OBJS		:= ex_spinlocks.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/spinlocks
clean:
	make -C ../.. subclean SUBDIRS=examples/spinlocks
endif

ex_spinlocks.c: rt_process.c
	ln -sf $< $@
