

MI_OBJS	+= ex_parport_sender.o \
	ex_parport_receiver.o

MIX_OBJS += rtai_parport.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=examples/parport
clean:
	make -C ../.. subclean SUBDIRS=examples/parport
endif


ex_parport_sender.c: sender.c
	ln -sf $< $@

ex_parport_receiver.c: receiver.c
	ln -sf $< $@

