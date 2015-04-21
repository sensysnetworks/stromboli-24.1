

EXTRA_CFLAGS := 

MI_OBJS += rtai_mupsex_controller.o

A_TARG := spvs

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=mups_examples/controller
clean:
	make -C ../.. subclean SUBDIRS=mups_examples/controller
endif

rtai_mupsex_controller.o: controller.o
	$(LD) -r -o $@ controller.o

