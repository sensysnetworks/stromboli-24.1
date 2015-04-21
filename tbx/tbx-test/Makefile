EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -lpthread

MI_OBJS := rtai_tbxtest_1.o rtai_tbxtest_2.o rtai_tbxtest_3.o q_test.o mod_stuff.o 

A_TARG = utest

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=tbx/tbx-test
clean:
	make -C ../.. subclean SUBDIRS=tbx/tbx-test
endif

rtai_tbxtest_1.c: rt_process_1.c
	ln -sf $< $@

rtai_tbxtest_2.c: rt_process_2.c
	ln -sf $< $@

rtai_tbxtest_3.c: rt_process_3.c
	ln -sf $< $@
