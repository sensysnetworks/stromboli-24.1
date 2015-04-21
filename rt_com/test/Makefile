
# Broken
#MI_OBJS += hello_world.o
#MI_OBJS += hello_world2.o
#MI_OBJS += testcomrtai.o

# testcom.c is for RTLinux
#MI_OBJS += testcom.o

ifdef TOPDIR
include $(TOPDIR)/Rules.make
else
all:
	make -C ../.. modules SUBDIRS=rt_com/test
clean:
	make -C ../.. subclean SUBDIRS=rt_com/test
endif
