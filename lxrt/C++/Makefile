
EXTRA_CXXFLAGS = -I ../
EXTRA_USER_CXXFLAGS = -I ../

EXTRA_USER_LIBS := -llxrt -lstdc++

U_CXXTARG  := test
test-objs := test.o

U_CXXTARG += test_mem
test_mem-objs := test_mem.o hrt.o

MI_OBJS  := ex_cplusplus.o
ifdef CONFIG_RTAI_FIFOS
ex_cplusplus-objs := FifoSrv.o task.o rt_process.o mod.o builtin.o
else
ex_cplusplus-objs := task.o rt_process.o mod.o builtin.o
endif

#M_CXXOBJS := FifoSrv.o task.o rt_process.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/C++
clean:
	make -C ../.. subclean SUBDIRS=lxrt/C++
endif

ex_cplusplus.o: $(ex_cplusplus-objs)
	$(LD) -r -o $@ $(ex_cplusplus-objs)


