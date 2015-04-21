
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -lpthread -lncurses

MI_OBJS := handler.o

A_TARG := pressa


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/pressa
clean:
	make -C ../.. subclean SUBDIRS=lxrt/pressa
endif


