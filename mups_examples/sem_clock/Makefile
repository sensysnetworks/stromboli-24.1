# Kernel Module list
MI_OBJS	:= rtai_mupsex_sem_clock.o \
	rtai_mupsex_sem_cmdclk.o \
	rtai_mupsex_sem_cmdcrn.o \
	rtai_mupsex_sem_dispclk.o

# User Application list
A_TARG 		:= keybrd screen

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=mups_examples/sem_clock
clean:
	make -C ../.. subclean SUBDIRS=mups_examples/sem_clock
endif

rtai_mupsex_sem_clock.c:	clock.c
	ln -sf $< $@

rtai_mupsex_sem_cmdclk.c:	cmdclk.c
	ln -sf $< $@

rtai_mupsex_sem_cmdcrn.c:	cmdcrn.c
	ln -sf $< $@

rtai_mupsex_sem_dispclk.c:	dispclk.c
	ln -sf $< $@

