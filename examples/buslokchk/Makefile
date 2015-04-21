
MI_OBJS := ex_buslokchk.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/buslokchk
clean:
	make -C ../.. subclean SUBDIRS=examples/buslokchk
endif

