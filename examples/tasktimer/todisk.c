/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#define NREC 100
#define RECSIZE 5000
#define FILSIZE 50000000
struct { int rec; char buf[RECSIZE - sizeof(int)];} record[NREC];

int main(void)
{
	int rtf, cmd, k;
	int size, fd, lost;
	struct sched_param mysched;

	mysched.sched_priority = 99;

	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERRORE SETTAGGIO SCHEDULER ");
		perror( "errno" );
		exit( 0 );
 	}       
                                                           
	if ((rtf = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}
	if ((cmd = open("/dev/rtf1", O_WRONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf1\n");
		exit(1);
	}
	printf("TRUNCATING\n");
	fd = open("dumpfile", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	write(cmd, &cmd, 1);
	printf("GO\n");
	size = 0;
	lost = 0;
	do {
		k = read(rtf, &record, sizeof(record));
		size += k;
		if (size != record[k/RECSIZE-1].rec) { 
			lost += record[k/RECSIZE-1].rec - size;
			printf("%d %d %d\n", size, record[k/RECSIZE-1].rec, lost);
			size = record[k/RECSIZE-1].rec;
		}
		write(fd, &record, k);
	} while(size < FILSIZE);
	write(cmd, &cmd, 1);
	close(fd);
	printf("END %d %d %d\n", size, record[k/RECSIZE-1].rec, lost);

	return 0;
}
