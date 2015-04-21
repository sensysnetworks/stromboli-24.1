MI_OBJS	+= ex_bits.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=bits/kbitstest
clean:
	make -C ../.. subclean SUBDIRS=bits/kbitstest
endif

ex_bits.c: rt_process.c
	ln -sf $< $@
