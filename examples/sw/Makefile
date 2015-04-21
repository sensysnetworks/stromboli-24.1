
MI_OBJS		:= ex_sw.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/sw
clean:
	make -C ../.. subclean SUBDIRS=examples/sw
endif

ex_sw.c: rt_process.c
	ln -sf $< $@

