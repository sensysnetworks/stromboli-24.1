
CFLAGS_ex_fp.o := $(CFLAGS_FP_ADD)

EXTRA_USER_LIBS := -lm

MI_OBJS := ex_fp.o

A_TARG	:= check

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/fp
clean:
	make -C ../.. subclean SUBDIRS=examples/fp
endif

ex_fp.c:  rt_process.c
	ln -sf $< $@

