
ALL_SUB_DIRS  := 
MOD_SUB_DIRS  := 
SUB_DIRS      := 

EXTRA_CFLAGS := -I../include -I.

test-objs := \
	rtai_posixtest_ptest.o \
	rtai_posixtest_rtaitest.o \
	rtai_posixtest_clockif_test.o \
	rtai_posixtest_mutextest.o \
	rtai_posixtest_mutexprio_test.o \
	rtai_posixtest_cond_brd_test.o \
	rtai_posixtest_condtest.o \
	rtai_posixtest_dyn_thread_create.o \
	rtai_posixtest_pqueuetest.o \
	rtai_posixtest_q_test.o \
	rtai_posixtest_simple_q1.o \
	rtai_posixtest_simple_q2.o \
	rtai_posixtest_simple_q3.o \
	rtai_posixtest_vsimple_q.o \
	rtai_posixtest_blq_test.o \
	rtai_posixtest_simple_blq.o \
	rtai_posixtest_toomanyqs.o \
	rtai_posixtest_mblq_test.o

ifneq ($(CONFIG_RTAI_TESTS),)
  MI_OBJS	+= $(test-objs)
endif


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=posix/test
clean:
	make -C ../.. subclean SUBDIRS=posix/test
endif

rtai_posixtest_%.c: %.c
	ln -sf $< $@

