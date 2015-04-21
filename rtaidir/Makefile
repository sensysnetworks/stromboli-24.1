
EXTRA_CFLAGS := -I.

ifeq ($(CONFIG_IRQ_PRIORITY),y)
  rtai-objs := rtai-nwa.o
  rtai-objs += rt_printk.o
  rtai-objs += proc.o
  rtai-objs += lib.o
else
  ifeq ($(CONFIG_PPC),y)
    rtai-objs := rtai-arch.o
    MIX_OBJS = rtai-arch.o
    # XXX We'll break x86 soon enough
    # rtai-objs += rt_printk.o
    # rtai-objs += proc.o
    # rtai-objs += lib.o
  endif
  ifeq ($(CONFIG_X86),y)
    rtai-objs := rtai-arch.o
    MIX_OBJS = rtai-arch.o
  endif
  ifeq ($(CONFIG_MIPS),y)
    rtai-objs := rtai-arch.o
    MIX_OBJS = rtai-arch.o
  endif
  ifeq ($(CONFIG_ARM),y)
    rtai-objs := rtai-arch.o
    MIX_OBJS = rtai-arch.o
    rtai-objs += rt_printk.o timer.o
#  MI_OBJS = rt_printk.o timer.o
  endif
  ifeq ($(CONFIG_ETRAX100LX),y)
   rtai-objs := rtai-arch.o
   MIX_OBJS = rtai-arch.o
   rtai-objs += rt_printk.o
  endif
  ifeq ($(CONFIG_ETRAX100LX_V2),y)
   rtai-objs := rtai-arch.o
   MIX_OBJS = rtai-arch.o
   rtai-objs += rt_printk.o
  endif
  ifeq ($(CONFIG_UCLINUX),y)
    rtai-objs := rtai-arch.o
    MIX_OBJS = rtai-arch.o
  endif
endif

M_OBJS = rtai.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=rtaidir
clean:
	make -C .. subclean SUBDIRS=rtaidir
endif

rtai.o:		$(rtai-objs)
	$(LD) -r -o $@ $(rtai-objs)

ifeq ($(CONFIG_RTAI_LINUX22),y)
rtai-arch.c: ../arch/$(ARCH)/rtai-22.c
	ln -sf $< $@
else
  ifeq ($(CONFIG_RTAI_PPC_EXPERIMENTAL),y)
rtai-arch.c: ../arch/$(ARCH)/rtai-exp.c
	ln -sf $< $@
  else
    ifeq ($(CONFIG_RTAI_ADEOS),y)
rtai-arch.c: ../arch/$(ARCH)/arti.c
	ln -sf $< $@
    else
      ifeq ($(CONFIG_ARM),y)
rtai-arch.c: ../arch/$(ARCH)/rtaisel.c
	ln -sf $< $@
      else      
rtai-arch.c: ../arch/$(ARCH)/rtai.c
	ln -sf $< $@
      endif
    endif
  endif
endif

ifeq ($(CONFIG_ARM),y)
timer.c: ../arch/arm/mach/timer.c
	ln -sf $< $@
endif
