ifndef LINUX_HOME
LINUX_HOME = /usr/src/linux
endif
RT_ARCH = $(shell test -f $(LINUX_HOME)/.config; echo $$?)
K_CFLAGS += -D__KERNEL__ -DMODULE
ifdef TESTVER
K_CFLAGS += -DLINUX_TESTVERSION=$(TESTVER)
endif

CFLAGS += -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strength-reduce -msoft-float $(K_CFLAGS) -c
CFLAGS_FP += -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strength-reduce $(K_CFLAGS) -c

ifeq ($(RT_ARCH), 0)
	include $(LINUX_HOME)/.config
endif

ifdef CONFIG_SMP
	CFLAGS += -D__SMP__
	CFLAGS_FP += -D__SMP__
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
