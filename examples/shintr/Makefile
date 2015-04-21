
MI_OBJS		:= ex_intr.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/shintr
clean:
	make -C ../.. subclean SUBDIRS=examples/shintr
endif

ex_intr.c: rt_process.c
	ln -sf $< $@

