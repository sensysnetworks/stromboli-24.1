
# list of all examples
examples := buslokchk condtest edf fastick fp frank jepplin \
  jitter_free_sw lockcpu mbx mbx_del msg_clock msgsw namedstuff \
  parport preempt prioinher res_sem resumefromintr rr sem_clock \
  sound stress sw switches sysreq tasktimer timer xmsgsw zentr_preempt

#list of examples that require FIFOs
ifndef CONFIG_RTAI_FIFOS
disable += fastick fp frank msg_clock parport preempt \
	resumefromintr sem_clock sound stress switches \
	tasktimer timer zentr_preempt
endif

# list of examples that require FP
ifndef CONFIG_RTAI_FPU_SUPPORT
ifdef CONFIG_RTAI_FIFOS
disable += tasktimer fp
endif
endif

# list of examples that require SMP
ifndef CONFIG_SMP
disable += lockcpu
endif

# list of examples that don't compile/work on PowerPC
ifdef CONFIG_PPC
ifdef CONFIG_RTAI_FIFOS
examples += ppc
disable += condtest parport sound zentr_preempt buslokchk timer
else
disable += condtest buslokchk timer
endif
endif

# list of examples that don't compile/work on MIPS
ifdef CONFIG_MIPS
ifdef CONFIG_RTAI_FIFOS
disable += condtest parport sound zentr_preempt buslokchk
else
disable += condtest buslokchk
endif
endif

# list of examples that don't compile/work on cris
ifdef CONFIG_ETRAX100LX
disable += sysreq sound lockcpu buslokchk parport
endif
ifdef CONFIG_ETRAX100LX_V2
disable += sysreq sound lockcpu buslokchk parport
endif 

ALL_SUB_DIRS  := $(examples)
ifdef CONFIG_RTAI_EXAMPLES
MOD_SUB_DIRS  := $(filter-out $(disable), $(examples))
SUB_DIRS      := $(filter-out $(disable), $(examples))
else
MOD_SUB_DIRS  := 
SUB_DIRS      := 
endif

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C .. modules SUBDIRS=examples
clean:
	make -C .. subclean SUBDIRS=examples
endif
