#
# This Makefile was generated automatically by lvmkmf.
# and then modified to meet RTAI LXRT environment
#
RTAI=/home/leibner/rtai/rtai-24.1.8-tleibner
CC=gcc -Wall -I$(RTAI) -I$(RTAI)/include -I$(RTAI)/lxrt
LD=gcc -rdynamic
LDFLAGS=-shared 
XFLAGS=-fPIC -O
CINDIR=/usr/local/lv51/cintools
CFLAGS=-I$(CINDIR) $(XFLAGS)
CINLIB=$(CINDIR)/libcin.a
MAKEGLUE=$(CINDIR)/makeglueLinux.awk
AS=gcc -fPIC -c

.SUFFIXES: .lsb .lsb~ $(SUFFIXES)

# Default rule to create an lsb from a C source file
.c.lsb:	; make $*.o
	$(LD) $(LDFLAGS) -o $*.tmp \
		$(CINDIR)/cin.o $(XLDFLAGS) $*.o $(CINLIB)
	$(CINDIR)/lvsbutil -c $* -t CIN -d `pwd`
	@rm -f $*.tmp

all:
	@echo "You must specify a target to make, for example, 'make mycin.lsb'"

