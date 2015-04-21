
MI_OBJS	+= ex_fastick_1.o ex_fastick_2.o

A_TARG	:= check


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/fastick
clean:
	make -C ../.. subclean SUBDIRS=examples/fastick
endif


ex_fastick_1.c:	rt_process1.c
	ln -sf $< $@

ex_fastick_2.c:	rt_process2.c
	ln -sf $< $@

