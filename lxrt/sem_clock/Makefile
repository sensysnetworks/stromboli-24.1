
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -lpthread

A_TARG := keybrd screen

U_TARG := clock
clock-objs := clock.o cmdclk.o cmdcrn.o dispclk.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/sem_clock
clean:
	make -C ../.. subclean SUBDIRS=lxrt/sem_clock
endif

