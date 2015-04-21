SRC = \
	rtai_scope.c \
	rtai_led.c \
	rtai_meter.c \
	memory.c \
	inp_square.c \
	inp_step.c \
	rtai_comedi_data.c \
	rtai_comedi_dio.c \
	peak.c \
	pcan.c \
	libpcan.c

OBJ = $(SRC:.c=.o)
LIBRARY = ulibsci.a
all: $(OBJ)

LINUX_HOME = /usr/src/linux
RTAI_HOME = /home/rtai4
SCI_HOME  = /usr/local/scicos
COMEDI_HOME  = /usr/src/comedi

INC_LNX    = -I$(LINUX_HOME)/include
INC_RTA    = -I$(RTAI_HOME)/include
INC_SCI    = -I$(SCI_HOME)/include
INC_COM    = -I$(COMEDI_HOME)/include

INC_RTAI   = -I. $(INC_RTA) $(INC_LNX) $(INC_SCI) $(INC_COM)
C_FLAGS    = -Wall -O2
%.o : %.c
	cc -c $(INC_RTAI) $(C_FLAGS) -o $@ $<
	ar r $(LIBRARY) $@
	rm $@

install:
	install *.a $(SCI_HOME)

clean:
	rm ulibsci.a
