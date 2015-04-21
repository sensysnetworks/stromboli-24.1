subdirs := examples

ALL_SUB_DIRS := $(subdirs) 
MOD_SUB_DIRS := 
SUB_DIRS     := 

ifdef CONFIG_RTAI_EXAMPLES
	MOD_SUB_DIRS += $(subdirs)
	SUB_DIRS     += $(subdirs)
endif

EXTRA_CFLAGS := -I. -I../watchdog/ -Wall
EXTRA_CXXFLAGS := -I. -I../watchdog/ -Wall

rtai_cpp-objs := init.o builtin.o rtai_wrapper.o linux_wrapper.o \
		 rtai_asm_wrapper.o module.o time.o count.o task.o \
		 sem.o mbx.o iostream.o mutex.o cond.o trace.o c++support.o

rtai_cpp_bits-objs := bits_init.o rtai_bits_wrapper.o bits.o
rtai_cpp_rtf-objs := rtf_init.o rtf.o
rtai_cpp_tbx-objs := tbx_init.o rtai_tbx_wrapper.o tbx.o
rtai_cpp_watchdog-objs := watchdog_init.o watchdog.o
rtai_cpp_com-objs := com_init.o com.o


ifneq ($(CONFIG_RTAI_CPLUSPLUS),)
  M_OBJS += rtai_cpp.o rtai_cpp_bits.o rtai_cpp_rtf.o \
	    rtai_cpp_tbx.o rtai_cpp_watchdog.o rtai_cpp_com.o
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=rtai_cpp
clean:
	make -C .. subclean SUBDIRS=rtai_cpp
endif

rtai_cpp.o: $(rtai_cpp-objs) crtbegin.o crtend.o crtsbegin.o crtsend.o crtmbegin.o crtmend.o
	$(LD) -r -o $@ $(rtai_cpp-objs)

rtai_cpp_bits.o: $(rtai_cpp_bits-objs) 
	$(LD) -r -o $@ $(rtai_cpp_bits-objs)

rtai_cpp_rtf.o: $(rtai_cpp_rtf-objs) 
	$(LD) -r -o $@ $(rtai_cpp_rtf-objs)

rtai_cpp_tbx.o: $(rtai_cpp_tbx-objs) 
	$(LD) -r -o $@ $(rtai_cpp_tbx-objs)

rtai_cpp_watchdog.o: $(rtai_cpp_watchdog-objs) 
	$(LD) -r -o $@ $(rtai_cpp_watchdog-objs)

rtai_cpp_com.o: $(rtai_cpp_com-objs) 
	$(LD) -r -o $@ $(rtai_cpp_com-objs)

crtsbegin.o : crt.c
	$(CC) $(CFLAGS) -DCRT_BEGIN -DNO_MOD_INIT -c crt.c -o crtsbegin.o
	
crtsend.o : crt.c
	$(CC) $(CFLAGS) -DCRT_END -DNO_MOD_INIT -c crt.c -o crtsend.o

crtbegin.o : crt.c
	$(CC) $(CFLAGS) -DCRT_BEGIN -c crt.c -o crtbegin.o
	
crtend.o : crt.c
	$(CC) $(CFLAGS) -DCRT_END -c crt.c -o crtend.o

crtmbegin.o : crt.c
	$(CC) $(CFLAGS) -DCRT_BEGIN -DUSE_MAIN -c crt.c -o crtmbegin.o

crtmend.o : crt.c 
	$(CC) $(CFLAGS) -DCRT_END -DUSE_MAIN -c crt.c -o crtmend.o
