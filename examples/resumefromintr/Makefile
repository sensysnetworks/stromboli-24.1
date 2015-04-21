
MI_OBJS := ex_resumefromintr.o

A_TARG	:= check

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/resumefromintr
clean:
	make -C ../.. subclean SUBDIRS=examples/resumefromintr
endif


ex_resumefromintr.c: rt_process.c
	ln -sf $< $@

