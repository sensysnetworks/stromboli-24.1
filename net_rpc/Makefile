
ifdef CONFIG_RTAI_SHMEM
ALL_SUB_DIRS := ktest rtnet uasync uresumefromintr utest usound usoundmsg
MOD_SUB_DIRS := ktest rtnet uasync uresumefromintr utest usound usoundmsg
else
ALL_SUB_DIRS :=
MOD_SUB_DIRS :=
endif

EXTRA_CFLAGS += -DURTNET=$(TOPDIR)/net_rpc/rtnet/urtnet
EXTRA_USER_CFLAGS += -I $(TOPDIR)/lxr

ifeq ($(CONFIG_RTAI_NETRPC),m)
  A_TARG := node
  M_OBJS += net_rpc.o scrambler.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=net_rpc
clean:
	make -C .. subclean SUBDIRS=net_rpc
endif

RTAI_DIR = /home/rtai-cvs/rtai4
hostpanel: hostpanel.c
	gcc -O2 -Wall -I$(RTAI_DIR)/include -o hostpanel hostpanel.c -lpthread
