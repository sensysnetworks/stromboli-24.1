all: ../rt_process.o

HOST=UNIX
BUILD=yes
SYS_TARGET_FILE = krtai.tlc
MAKECMD         = make

MODEL           = test
MODULES         = test_data.c 
MAKEFILE        = test.mk
MATLAB_ROOT     = /usr/local/matlab6p5
S_FUNCTIONS     = sfun_rtai_scope.c
SOLVER          = ode4.c
NUMST           = 2
TID01EQ         = 1
NCSTATES        = 1
COMPUTER        = GLNX86
BUILDARGS       = ADD_MDL_NAME_TO_GLOBALS=1
MULTITASKING    = 0

CC         = gcc -c 

LINUX_HOME = /usr/src/linux
RTAI_HOME  = /home/rtai4
RTW_R      = $(MATLAB_ROOT)/rtw/c
RTAI_RTW   = $(MATLAB_ROOT)/rtw/c/rtai

INC_LNX    = -I$(LINUX_HOME)/include
INC_RTA    = -I$(RTAI_RTW) -I$(RTAI_HOME)/include
INC_MAT    = -I$(MATLAB_ROOT)/extern/include
INC_SIM    = -I$(MATLAB_ROOT)/simulink/include
INC_RTW    = -I$(RTW_R)/src -I$(RTW_R)/libsrc

INC_RTAI   = $(INC_RTA) $(INC_LNX) 
INC_MATLAB = $(INC_LNX) $(INC_MAT) $(INC_SIM) $(INC_RTW)

KERNFLAGS  = -D__KERNEL__
GCCFLAGS   = -O2 -Wall
MODFLAGS   = -DMODULE
NOVERSION  = -D__NO_VERSION
CC_FLAGS   = -DMODEL=$(MODEL) -DRT -DNUMST=$(NUMST) -DUNIX \
             -DTID01EQ=$(TID01EQ) -DNCSTATES=$(NCSTATES) \
             -DMT=$(MULTITASKING) -DKRTAI

CFLAGS     = -DRT $(GCCFLAGS) $(NOVERSION) -DNO_PRINTF -ffast-math -mhard-float

LD_FLAGS   = -r
RTW_LIB    = librtw.a
RTAI_LIB   = librtai.a

SRC_MAIN   = krtmain.c
SRC_INTER  = krtai2rtw.c
SRCS       = $(S_FUNCTIONS)
SRCS_RTW   = rt_sim.c $(SOLVER)

PROGRAM    = ../rt_process.o

OBJS       = $(SRC_MAIN:.c=.o) $(SRC_INTER:.c=.o) $(SRCS:.c=.o) $(MODEL).o $(MODULES:.c=.o) $(SRCS_RTW:.c=.o) 

$(SRC_MAIN:.c=.o):$(RTAI_RTW)/$(SRC_MAIN)
	$(CC) $(INC_RTAI) $(GCCFLAGS) $(KERNFLAGS) $(MODFLAGS) -o $@ $<

$(SRC_INTER:.c=.o):$(RTAI_RTW)/$(SRC_INTER)
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(GCCFLAGS) $(NOVERSION) -o $@ $<

$(MODEL).o: $(MODEL).c
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(GCCFLAGS) $(NOVERSION) -ffast-math -o $(MODEL).o $(MODEL).c

$(MODEL)_data.o: $(MODEL)_data.c
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(GCCFLAGS) $(NOVERSION) -ffast-math -o $(MODEL)_data.o $(MODEL)_data.c

%.o: $(RTAI_RTW)/devices/%.c
	$(CC) $(INC_MATLAB) $(INC_RTAI) $(GCCFLAGS) $(CC_FLAGS) $(NOVERSION) -ffast-math $<

%.o: $(RTW_R)/src/%.c
	$(CC) $(INC_MATLAB) $(GCCFLAGS) $(CC_FLAGS) $(NOVERSION) $<

%.o : $(MATLAB_ROOT)/toolbox/comm/commsfun/%.c
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(NOVERSION)$(CFLAGS)   $<

%.o : $(MATLAB_ROOT)/toolbox/dspblks/dspmex/%.c
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(NOVERSION) $(CFLAGS)  $<

%.o : $(MATLAB_ROOT)/toolbox/fixpoint/%.c
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(NOVERSION) $(CFLAGS)  $<

%.o : $(MATLAB_ROOT)/toolbox/fuzzy/fuzzy/src/%.c
	$(CC) $(INC_MATLAB) $(CC_FLAGS) $(NOVERSION) $(CFLAGS) $<

%.o : $(MATLAB_ROOT)/simulink/src/%.c
	$(CC) $(INC_MATLAB) $(GCCFLAGS) $(CC_FLAGS) $(NOVERSION) $<

$(RTW_LIB) : 
	@echo "Creating $@ from $(RTW_R)/libsrc/*.c"
	@\rm -f $@
	@for file in $(RTW_R)/libsrc/*.c; do \
	  echo "$(CC) $(CFLAGS) $$file"; \
	  $(CC) $(INC_MATLAB) $(CFLAGS) $$file; \
	  ofile=`echo $$file | sed -e 's/\.c/\.o/g' -e 's|/.*/||g'`; \
	  ar r $@ $$ofile; \
	  \rm -f $$ofile; \
	done

$(RTAI_LIB) : 
	@echo "Creating $@ from $(RTAI_RTW)/klib/*.c"
	@\rm -f $@
	@for file in $(RTAI_RTW)/lib/*.c; do \
	  echo "$(CC) $(INC_RTAI) $(GCCFLAGS) $(KERNFLAGS) $(MODFLAGS) $$file"; \
	  $(CC) $(INC_RTAI) $(GCCFLAGS) $(KERNFLAGS) $(MODFLAGS) $$file; \
	  ofile=`echo $$file | sed -e 's/\.c/\.o/g' -e 's|/.*/||g'`; \
	  ar r $@ $$ofile; \
	  \rm -f $$ofile; \
	done

$(PROGRAM): $(OBJS) $(RTW_LIB) $(RTAI_LIB)
	ld $(LD_FLAGS) -o $@ $(OBJS) $(RTW_LIB) $(RTAI_LIB)
	@echo "### Created executable: $(PROGRAM)"

clean:
	rm -f $(OBJS) *~ $(PROGRAM) $(RTW_LIB)

$(OBJS): $(MAKEFILE) rtw_proj.tmw
