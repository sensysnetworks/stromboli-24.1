
OBJS = Fl_Scope.o \
       Fl_Scope_Window.o \
       Fl_Led.o \
       Fl_Led_Window.o \
       Fl_Meter.o \
       Fl_Meter_Window.o \
       Fl_Synchronoscope.o \
       Fl_Synchronoscope_Window.o

TARGS = xrtailab

#CC = gcc-3.2.3
CFLAGS := $(shell efltk-config --cxxflags)
LFLAGS := $(shell efltk-config --ldflags --use-gl --multithread)

all: $(TARGS)

%.o: %.cpp %.h
	$(CC) $(CFLAGS) -c -o $@ $<

xrtailab: xrtailab.cpp $(OBJS)
	$(CC) -I. -I../include $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	@rm -f $(OBJS) $(TARGS)
