ALL_SUB_DIRS := 
MOD_SUB_DIRS := 
SUB_DIRS     := 

ifdef CONFIG_RTAI_CPLUSPLUS
SUB_DIRS     += 
MOD_SUB_DIRS += 
endif

EXTRA_CFLAGS := -I. -I$(TOPDIR)/rtai_cpp/ 
EXTRA_CXXFLAGS := -I. -I$(TOPDIR)/rtai_cpp/

rtai_cpp_simple-objs := $(TOPDIR)/rtai_cpp/crtsbegin.o \
			modinfo.o simple.o \
			$(TOPDIR)/rtai_cpp/crtsend.o


ifneq ($(CONFIG_RTAI_CPLUSPLUS),)
  M_OBJS += rtai_cpp_simple.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../../.. modules SUBDIRS=rtai_cpp/examples/simple
clean:
	make -C ../../.. subclean SUBDIRS=rtai_cpp/examples/simple
endif

rtai_cpp_simple.o: $(rtai_cpp_simple-objs) 
	$(LD) -r -o $@ $(rtai_cpp_simple-objs)

