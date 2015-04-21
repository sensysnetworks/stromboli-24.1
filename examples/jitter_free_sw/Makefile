

MI_OBJS	+= ex_jitterfree.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/jitter_free_sw
clean:
	make -C ../.. subclean SUBDIRS=examples/jitter_free_sw
endif

ex_jitterfree.c:	rt_process.c
	ln -sf $< $@

