
EXTRA_USER_CFLAGS := -I ..
EXTRA_USER_LIBS := -llxrt
#EXTRA_USER_CFLAGS := -I$(LINUXDIR)/include

CFLAGS_leds_lib.o += -DKEEP_INLINE -fkeep-inline-functions

L_TARG := libleds.a
#B_TARG := rt_leds
#B_TARG := rt_leds test
M_OBJS := leds_lxrt.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/leds
clean:
	make -C ../.. subclean SUBDIRS=lxrt/leds
endif

libleds.a: leds_lib.o
	ar rs $@ $<
	ranlib $@


#leds_lib.o: leds_lib.c
#	$(CC) $(USER_CFLAGS) -I$(LINUXDIR)/include -DKEEP_INLINE \
#		-fkeep-inline-functions -c -o $@ $<

