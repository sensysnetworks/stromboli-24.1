
MOD_LIST_NAME := MISC_MODULES

EXTRA_CFLAGS := -I. -I../../

M_OBJS	:= lock_task.o lock_thread.o switch_cpu.o

A_TARG	:= lock_process

EXTRA_USER_LDFLAGS := -lm

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/lockcpu
clean:
	make -C ../.. subclean SUBDIRS=examples/lockcpu
endif
