
MI_OBJS	+= ex_mbx.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/mbx
clean:
	make -C ../.. subclean SUBDIRS=examples/mbx
endif

ex_mbx.c:	rt_process.c
	ln -sf $< $@

