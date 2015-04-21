
ALL_SUB_DIRS :=
MOD_SUB_DIRS :=
SUB_DIRS     :=

# This is a totally bogus way of handling nommu.  It really needs
# to be merged into one file.
ifeq ($(CONFIG_RTAI_FIFOS),m)
ifeq ($(CONFIG_RTAI_LINUX24),y)
ifeq ($(CONFIG_UCLINUX),y)
M_OBJS       := rtai_shm_nommu.o
else
M_OBJS       := rtai_shm.o
endif
endif
endif
#MI_OBJS      := rtai_shm_tmp.o
#MI_OBJS     += kvmem.o
MIX_OBJS     := 



ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=shmem
clean:
	make -C .. subclean SUBDIRS=shmem
endif

#rtai_shm.o: rtai_shm_tmp.o
#	ln -s $< $@



