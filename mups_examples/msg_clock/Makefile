
EXTRA_CFLAGS := -I.

MI_OBJS	+= rtai_mupsex_msg_clock.o \
	rtai_mupsex_msg_cmdclk.o \
	rtai_mupsex_msg_cmdcrn.o \
	rtai_mupsex_msg_dispclk.o


A_TARG := keybrd screen

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=mups_examples/msg_clock
clean:
	make -C ../.. subclean SUBDIRS=mups_examples/msg_clock
endif


rtai_mupsex_msg_clock.c:	clock.c
	ln -sf $< $@

rtai_mupsex_msg_cmdclk.c:	cmdclk.c
	ln -sf $< $@

rtai_mupsex_msg_cmdcrn.c:	cmdcrn.c
	ln -sf $< $@

rtai_mupsex_msg_dispclk.c:	dispclk.c
	ln -sf $< $@

