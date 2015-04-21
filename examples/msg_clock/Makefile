
MI_OBJS	+= clock.o
MIX_OBJS += cmdclk.o cmdcrn.o dispclk.o

A_TARG := keybrd screen

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/msg_clock
clean:
	make -C ../.. subclean SUBDIRS=examples/msg_clock
endif


