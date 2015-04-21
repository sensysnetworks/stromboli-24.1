
ALL_SUB_DIRS  := 
MOD_SUB_DIRS  := 
SUB_DIRS      := 

EXTRA_CFLAGS := -I../include

ifneq ($(CONFIG_RTAI_POSIX),)
M_OBJS	+= rtai_pthread.o
M_OBJS	+= rtai_pqueue.o
M_OBJS	+= rtai_utils.o
endif

pthread_objs := rtai_pthread_tmp.o
pqueue_objs := rtai_pqueue_tmp.o
utils_objs := rtai_utils_tmp.o


ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=posix/src
clean:
	make -C ../.. subclean SUBDIRS=posix/src
endif

rtai_pthread_tmp.o:     rtai_pthread.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

rtai_pthread.o: $(pthread_objs)
	$(LD) -r -o $@ $(pthread_objs)

rtai_pqueue_tmp.o:      rtai_pqueue.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

rtai_pqueue.o:  $(pqueue_objs)
	$(LD) -r -o $@ $(pqueue_objs)

rtai_utils_tmp.o:       rtai_utils.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

rtai_utils.o:   $(utils_objs)
	$(LD) -r -o $@ $(utils_objs)




