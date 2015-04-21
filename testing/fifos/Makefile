
ALL_SUB_DIRS := anothertest
MOD_SUB_DIRS := anothertest
SUB_DIRS     := anothertest

MI_OBJS      := rtai_test_fifos_1.o rtai_test_fifos_2.o rtai_test_fifos_3.o

A_TARG       := regression regression2 regression3

EXTRA_USER_LIBS := -lpthread
#EXTRA_USER_CFLAGS := -DRTAI_RTF_NAMED


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=testing/fifos
clean:
	make -C ../.. subclean SUBDIRS=testing/fifos
endif


rtai_test_fifos_1.c: regression.c
	ln -sf $< $@

rtai_test_fifos_2.c: regression2.c
	ln -sf $< $@

rtai_test_fifos_3.c: regression3.c
	ln -sf $< $@


