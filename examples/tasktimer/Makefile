
MI_OBJS		:= ex_tasktimer.o

A_TARG 		:= check todisk


CFLAGS_ex_tasktimer.o := $(CFLAGS_ADD_FP)


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/tasktimer
clean:
	make -C ../.. subclean SUBDIRS=examples/tasktimer
endif


ex_tasktimer.c: rt_process.c
	ln -sf $< $@

