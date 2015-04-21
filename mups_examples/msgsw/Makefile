
EXTRA_CFLAGS := -I.

MI_OBJS	+= rtai_mupsex_msgsw.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=mups_examples/msgsw
clean:
	make -C ../.. subclean SUBDIRS=mups_examples/msgsw
endif

rtai_mupsex_msgsw.c:	rt_process.c
	ln -sf $< $@

