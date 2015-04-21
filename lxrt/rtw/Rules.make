
ifndef LINUX_HOME
LINUX_HOME = /usr/src/linux
endif
RT_ARCH = $(shell test -f $(LINUX_HOME)/.config; echo $$?)
K_CFLAGS += -D__KERNEL__ -DMODULE
ifdef TESTVER
K_CFLAGS += -DLINUX_TESTVERSION=$(TESTVER)
endif

# Comment out the line below if you don't want to use the dynamic memory manager
# Note that in such a case POSIX cannot be used because the schedulers are not
# prepared for it. POSIX wants the dynamic memory menager.
# So I changed the default to yes. Also, uncomment CONFIG_SMP_8254 to force
# the installation of the 8254 SMP scheduler if you compile a SMP kernel
# for a UP machine. PC.

CONFIG_RTAI_DYN_MM=y;
#CONFIG_SMP_8254=y;

ifdef CONFIG_RTAI_DYN_MM
K_CFLAGS += -DCONFIG_RTAI_DYN_MM
endif

K_CFLAGS += -DCONFIG_RTAI_FPU_SUPPORT

CFLAGS += -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strength-reduce -msoft-float $(K_CFLAGS) -c
CFLAGS_FP += -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strength-reduce $(K_CFLAGS) -c

ifeq ($(RT_ARCH), 0)
	include $(LINUX_HOME)/.config
endif

ifdef CONFIG_SMP
	CFLAGS += -D__SMP__
	CFLAGS_FP += -D__SMP__
endif

ifdef CONFIG_M386
	CFLAGS += -DCPU=386
	CFLAGS_FP += -DCPU=386
endif

ifdef CONFIG_M486
	CFLAGS += -DCPU=486
	CFLAGS_FP += -DCPU=486
endif

ifdef CONFIG_M586
	CFLAGS += -DCPU=586
	CFLAGS_FP += -DCPU=586
endif

ifdef CONFIG_M586TSC
	CFLAGS += -DCPU=586
	CFLAGS_FP += -DCPU=586
endif

ifdef CONFIG_M686
	CFLAGS += -DCPU=686
	CFLAGS_FP += -DCPU=686
endif
