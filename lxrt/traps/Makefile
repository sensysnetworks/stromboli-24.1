
EXTRA_USER_CFLAGS := -I .. -I ../leds
EXTRA_USER_LIBS := ../leds/libleds.a -llxrt

MI_OBJS := rt_process.o rt_multi.o

U_TARG += test_sft
test_sft-objs := test_sft.o

U_TARG += addr_space
addr_space-objs := addr_space.o

U_TARG += test_hrt
test_hrt-objs := test_hrt.o

U_TARG += lxrt_multi
lxrt_multi-objs := lxrt_multi.o
lxrt_multi-libs := -lpthread


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=lxrt/traps
clean:
	make -C ../.. subclean SUBDIRS=lxrt/traps
endif


