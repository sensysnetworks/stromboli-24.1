#
# Makefile for softfloat library
#
# Include the arch/processor specific include file
#

ALL_SUB_DIRS  := 
MOD_SUB_DIRS  := 
SUB_DIRS      := 


libs_objs 	= softfloat.o libgcc.o

ifeq ($(CONFIG_ARM),y)
libs_objs	+= fplib_glue.o
endif

ifneq ($(CONFIG_RTAI_SFLIB),)
include $(ARCH)/$(PROCESSOR)/Makefile
M_OBJS		+= rtai_libsf.o
MIX_OBJS	= libsf.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=libsf
clean:
	make -C .. subclean SUBDIRS=libsf
endif

rtai_libsf.o: $(libs_objs) libsf.o
	$(LD) -r -o $@ $(libs_objs) libsf.o

softfloat.o:	softfloat/bits64/softfloat.c
	$(CC) $(CFLAGS) $(CFLAGS_FP_ADD) $(EXTRA_CFLAGS) -c -o $@ $<

ifeq ($(CONFIG_ARM),y)
fplib_glue.o: $(ARCH)/fplib_glue.S
	$(CC) $(CFLAGS) $(CFLAGS_FP_ADD) $(EXTRA_CFLAGS) -c -o $@ $<
endif

libgcc.o: libgcc/libgcc2.c
	$(CC) $(CFLAGS) $(CFLAGS_FP_ADD) $(EXTRA_CFLAGS) -c -o $@ $<	

