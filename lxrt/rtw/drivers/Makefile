
MI_OBJS	:= ni_pci.o ni_pci_lxrt.o ni_e.o ni_e_lxrt.o

A_TARG := ni_pci_list

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../../.. modules SUBDIRS=lxrt/rtw/drivers
clean:
	make -C ../../.. subclean SUBDIRS=lxrt/rtw/drivers
endif
