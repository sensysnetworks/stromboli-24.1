

A_TARG := rtifconfig

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=rtnet/tools
clean:
	make -C ../.. subclean SUBDIRS=rtnet/tools
endif

