
MI_OBJS	+= ex_xmsgsw.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/xmsgsw
clean:
	make -C ../.. subclean SUBDIRS=examples/xmsgsw
endif

