
MI_OBJS	+= ex_jepplin.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/jepplin
clean:
	make -C ../.. subclean SUBDIRS=examples/jepplin
endif

ex_jepplin.c:	rt_process.c
	ln -sf $< $@

