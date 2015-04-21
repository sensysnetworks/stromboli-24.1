
ALL_SUB_DIRS := fifos
MOD_SUB_DIRS := 
SUB_DIRS     := 

ifdef CONFIG_RTAI_TESTS
ifdef CONFIG_RTAI_FIFOS
MOD_SUB_DIRS += fifos
SUB_DIRS     += fifos
endif
# rtai_test_fifos_1.o has been disabled temporarily
ifdef CONFIG_RTAI_SHMEM
ifdef CONFIG_RTA_FIFOS
MI_OBJS      := rtai_test_shmem_1.o rtai_test_sched_1.o
A_TARG       := test_shmem_1
else
MI_OBJS      := rtai_test_sched_1.o
A_TARG       :=
endif
else
MI_OBJS      := rtai_test_sched_1.o
A_TARG       := 
endif
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=testing
clean:
	make -C .. subclean SUBDIRS=testing
endif


