
MI_OBJS		:= ex_rwlocks.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/rwlocks
clean:
	make -C ../.. subclean SUBDIRS=examples/rwlocks
endif

ex_rwlocks.c: rt_process.c
	ln -sf $< $@

