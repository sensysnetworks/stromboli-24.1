

MI_OBJS		+= ex_zenpreempt.o

A_TARG	:= preempt

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/zentr_preempt
clean:
	make -C ../.. subclean SUBDIRS=examples/zentr_preempt
endif

ex_zenpreempt.c: preempt.c
	ln -sf $< $@

