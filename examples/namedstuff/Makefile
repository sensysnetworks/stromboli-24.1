
MI_OBJS := ex_named.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/namedstuff
clean:
	make -C ../.. subclean SUBDIRS=examples/namedstuff
endif

ex_named.c: rt_process.c
	ln -sf $< $@

