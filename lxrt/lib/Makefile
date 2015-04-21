
ALL_SUB_DIRS :=
MOD_SUB_DIRS :=
SUB_DIRS     :=

EXTRA_CFLAGS := -fkeep-inline-functions \
	$(CDEBUG) -fPIC -O2 -Wall -c

L_TARG       := liblxrt.a
BI_OBJS      := init_sched.o rtai_lxrt.o touchall.o



ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/lib
clean:
	make -C ../.. subclean SUBDIRS=lxrt/lib
endif


liblxrt.a: $(BI_OBJS)
	$(AR) rs $@ $^
	ranlib $@

