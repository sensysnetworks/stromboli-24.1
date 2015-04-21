
ALL_SUB_DIRS :=
MOD_SUB_DIRS :=
SUB_DIRS     :=

EXTRA_CFLAGS := -fkeep-inline-functions \
	$(CDEBUG) -fPIC -O2 -Wall -c

EXTRA_USER_CFLAGS := -I/usr/src/comedi/include

L_TARG       := libcomedilxrt.a
BI_OBJS      := rtai_comedi_lxrt.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=comedi_lxrt/lib
clean:
	make -C ../.. subclean SUBDIRS=comedi_lxrt/lib
endif


libcomedilxrt.a: $(BI_OBJS)
	$(AR) rs $@ $^
	ranlib $@

