
allobjs	:= test_kctest.o test_ktasklet.o test_kuctest.o test_ktest.o
alltarg	:= btest stest uctest utasklet utest

ifndef CONFIG_RTAI_SHMEM
disableobjs += test_kctest.o test_kuctest.o
disabletarg += uctest
endif

MI_OBJS := $(filter-out $(disableobjs), $(allobjs))

ifneq ($(CONFIG_RTAI_LXRT),)
A_TARG	:= $(filter-out $(disabletarg), $(alltarg))
endif

CFLAGS_test_kctest.o := $(CFLAGS_FP_ADD)
CFLAGS_test_kuctest.o := $(CFLAGS_FP_ADD)

EXTRA_USER_LIBS := -lpthread

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=tasklets/tests
clean:
	make -C ../.. subclean SUBDIRS=tasklets/tests
endif

test_ktest.c: ktest.c
	ln -sf $< $@

test_ktasklet.c: ktasklet.c
	ln -sf $< $@

test_kctest.c: kctest.c
	ln -sf $< $@

test_kuctest.c: kuctest.c
	ln -sf $< $@

