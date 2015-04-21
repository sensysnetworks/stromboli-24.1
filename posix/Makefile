
ifeq ($(CONFIG_RTAI_POSIX),m)
ALL_SUB_DIRS  := src test
MOD_SUB_DIRS  := src test
SUB_DIRS      := src test
endif

EXTRA_CFLAGS := 


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=posix
clean:
	make -C .. subclean SUBDIRS=posix
endif


