
MI_OBJS += ex_stress.o

A_TARG := checkl checkp


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/stress
clean:
	make -C ../.. subclean SUBDIRS=examples/stress
endif


ex_stress.c: rt_process.c
	ln -sf $< $@

