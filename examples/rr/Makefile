
MI_OBJS := ex_rr.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/rr
clean:
	make -C ../.. subclean SUBDIRS=examples/rr
endif

ex_rr.c: rt_process.c
	ln -sf $< $@

