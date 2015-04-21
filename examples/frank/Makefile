
MI_OBJS	+= ex_frank.o

A_TARG := frank_app


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/frank
clean:
	make -C ../.. subclean SUBDIRS=examples/frank
endif

ex_frank.c:	frank_module.c
	ln -sf $< $@


