

EXTRA_CFLAGS	= -DRTAI_PROC -DRTAI_RTF_NAMED -DRTAI_CONIO

MI_OBJS += task.o
A_TARG += check


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../../.. modules SUBDIRS=testing/fifos/anothertest
clean:
	make -C ../../.. subclean SUBDIRS=testing/fifos/anothertest
endif
