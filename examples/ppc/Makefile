# Kernel Module list
ifdef CONFIG_8xx
ifdef CONFIG_CPM_MULTILEVEL_IRQ
MI_OBJS		:= cpm_irqs.o
endif
endif
ifdef CONFIG_8260
MI_OBJS		:= cpm_irqs.o
endif
# User Application list
A_TARG 		:=

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	dir=`pwd | awk -F "/" '{print $$NF}'`; \
	make -C ../.. modules userspace SUBDIRS=examples/$$dir
endif

clean:
	rm -f $(A_TARG) *.o
