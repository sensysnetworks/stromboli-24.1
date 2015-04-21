
ALL_SUB_DIRS  := controller msg_clock msgsw sem_clock sw

EXAMPLE_DIRS  := 
ifdef CONFIG_X86
	ifdef CONFIG_RTAI_SCHED_MUP
		ifdef CONFIG_RTAI_FIFOS
			EXAMPLE_DIRS  += msg_clock msgsw sem_clock sw
		else
			EXAMPLE_DIRS  += msgsw sw
		endif
		ifdef CONFIG_RTAI_FPU_SUPPORT
			ifdef CONFIG_RTAI_SHMEM
				ifdef CONFIG_RTAI_FIFOS
					EXAMPLE_DIRS  += controller
				endif
			endif
		endif
	endif
endif

MOD_SUB_DIRS  := $(EXAMPLE_DIRS)
APP_SUB_DIRS  := $(EXAMPLE_DIRS)
SUB_DIRS      := $(EXAMPLE_DIRS)

EXTRA_CFLAGS := 


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=mups_examples
clean:
	make -C .. subclean SUBDIRS=mups_examples
endif



