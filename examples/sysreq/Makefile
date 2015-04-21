
MI_OBJS	+= ex_sysreq.o

A_TARG 	:= check

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/sysreq
clean:
	make -C ../.. subclean SUBDIRS=examples/sysreq
endif

ex_sysreq.c: rt_process.c
	ln -sf $< $@

