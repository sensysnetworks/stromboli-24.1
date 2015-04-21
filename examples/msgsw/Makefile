
MI_OBJS	+= ex_msgsw.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/msgsw
clean:
	make -C ../.. subclean SUBDIRS=examples/msgsw
endif

ex_msgsw.c:	rt_process.c
	ln -sf $< $@

