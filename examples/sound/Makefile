
MI_OBJS	+= ex_sound.o

A_TARG 		:= check


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/sound
clean:
	make -C ../.. subclean SUBDIRS=examples/sound
endif

ex_sound.c: rt_process.c
	ln -sf $< $@

