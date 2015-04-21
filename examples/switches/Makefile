
MI_OBJS		:= ex_switches.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/switches
clean:
	make -C ../.. subclean SUBDIRS=examples/switches
endif

ex_switches.c:	rt_process.c
	ln -sf $< $@

