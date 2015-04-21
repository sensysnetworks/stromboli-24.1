all: genconf_io

SRC = genconf_io.c 
OBJ = $(SRC:.c=.o)
CC  = cc -g 

%.o : %.c
	$(CC) -c -o $@ $<

genconf_io : $(OBJ)
	$(CC) -o $@ $(OBJ)

clean:
	rm -f *.o genconf_io
