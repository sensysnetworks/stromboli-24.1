
MI_OBJS		:= ex_timer.o

A_TARG 		:= check

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/timer
clean:
	make -C ../.. subclean SUBDIRS=examples/timer
endif

ex_timer.c: rt_process.c
	ln -sf $< $@

