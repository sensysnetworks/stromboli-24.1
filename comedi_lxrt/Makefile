
EXTRA_CFLAGS := -I/usr/src/comedi/include

EXTRA_USER_CFLAGS := -I/usr/src/comedi/include
EXTRA_USER_LDFLAGS := -L./lib
#comment out the line below to use libcomedilxrt
#EXTRA_USER_LIBS := -lcomedilxrt

subdirs := lib

ALL_SUB_DIRS := $(subdirs)

ifneq ($(CONFIG_RTAI_COMEDI_LXRT),)
	SUB_DIRS     += $(subdirs)
	MOD_SUB_DIRS += $(subdirs)
	M_OBJS += rtai_comedi_lxrt.o
	A_TARG += tests testa
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=comedi_lxrt
clean:
	make -C .. subclean SUBDIRS=comedi_lxrt
endif
