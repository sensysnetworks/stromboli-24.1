
MI_OBJS := ex_preempt.o

A_TARG	:= check


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/preempt
clean:
	make -C ../.. subclean SUBDIRS=examples/preempt
endif

ex_preempt.c:	rt_process.c
	ln -sf $< $@

