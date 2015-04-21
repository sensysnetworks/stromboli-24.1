
A_TARG := find_child_pids

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=testing/testsuite
clean:
	make -C ../.. subclean SUBDIRS=testing/testsuite
endif

