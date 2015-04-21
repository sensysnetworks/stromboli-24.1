ALL_SUB_DIRS := 
MOD_SUB_DIRS := 
SUB_DIRS     := 

ifdef CONFIG_RTAI_CPLUSPLUS
SUB_DIRS     += 
MOD_SUB_DIRS += 
endif

EXTRA_CFLAGS := -I. -I$(TOPDIR)/rtai_cpp/ 
EXTRA_CXXFLAGS := -I. -I$(TOPDIR)/rtai_cpp/

rtai_cpp_test-objs := 	$(TOPDIR)/rtai_cpp/crtmbegin.o \
			modinfo.o test.o \
			$(TOPDIR)/rtai_cpp/crtmend.o


ifneq ($(CONFIG_RTAI_CPLUSPLUS),)
  M_OBJS += rtai_cpp_test.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../../.. modules SUBDIRS=rtai_cpp/examples/test
clean:
	make -C ../../.. subclean SUBDIRS=rtai_cpp/examples/test
endif

rtai_cpp_test.o: $(rtai_cpp_test-objs) 
	$(LD) -r -o $@ $(rtai_cpp_test-objs)

