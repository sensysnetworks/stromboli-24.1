# Kernel Module list
MI_OBJS		:= rtai_mupsex_sw.o
# User Application list
A_TARG 		:=

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=mups_examples/sw
clean:
	make -C ../.. subclean SUBDIRS=mups_examples/sw
endif


rtai_mupsex_sw.c:	rt_process.c
	ln -sf $< $@

