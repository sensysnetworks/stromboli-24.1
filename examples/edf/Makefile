
MI_OBJS := ex_edf.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/edf
clean:
	make -C ../.. subclean SUBDIRS=examples/edf
endif

ex_edf.c: rt_process.c
	ln -sf $< $@

