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
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>

#include <rtai_fifos.h>
#include <rtai_shm.h>
#include <asm/rtai.h>
#include "controller.h"

static void init_mat(RTYPE *a, RTYPE *b, int nr, int nc)
{
	int i, k;

// A generic matrix, just to check.
	for (i = 0; i < nc; i++) {
		b[i] = RNUM;
	}
	for (i = 0; i < nr; i++) {
		for (k = 0; k < nc; k++) {
			a[k] = (k + 1)*RNUM;
		}
		a += nc;
	}
}

int main(void)
{
	int shmfif, synfif, file, nrecw, loops, recsize, nrb, ofst, fsize, lost;
	int *buf_base, *buf_frec, *buf_lrec;
	RTYPE *slope_to_pos_mat, *slopes, *pos;
	struct shm_control_block *shm;
	struct sched_param mysched;
	struct exec_info info;

	mysched.sched_priority = 99;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		fprintf(stderr, "ERROR IN LINUX POSIX SCHEDULER SET UP\n");
		exit(1);
	}       
	mlockall(MCL_CURRENT | MCL_FUTURE);

	if ((synfif = open(SYNDEV, O_WRONLY)) < 0) {
		fprintf(stderr, "ERROR OPENING FIFO %d\n", SYNFIF);
		exit(1);
	}

	if ((shmfif = open(SHMDEV, O_RDONLY)) < 0) {
		fprintf(stderr, "ERROR OPENING FIFO %d\n", SHMFIF);
		exit(1);
	}
	
	recsize = (RECSIZE + sizeof(int) - 1)/sizeof(int);
	nrb = (NROWS + NR_RT_CPUS - 1)/NR_RT_CPUS;
	
	if (!(shm = rtai_malloc(nam2num(SHMNAM), 
				sizeof(struct shm_control_block) + 
				MAXBUFREC*recsize*sizeof(int) + 
				(NCOLS + 1)*(nrb*NR_RT_CPUS + 1)*sizeof(RTYPE) +
				sizeof(int) + sizeof(RTYPE)))) {
		fprintf(stderr, "CANNOT ALLOCATE SHARED MEMORY\n");
		exit(1);
	}       

	shm->run     = 0;
	shm->maxrec  = MAXBUFREC;
	shm->recsize = recsize;
	shm->nrec = shm->nrecw = 0;
	shm->frec = shm->lrec  = 1;
	ofst = (sizeof(struct shm_control_block) + sizeof(int) - 1)/sizeof(int);
	shm->buf_base_ofst = ofst;
	buf_frec = buf_lrec = buf_base = (int *)shm + ofst;
	ofst = (ofst + MAXBUFREC*recsize)*sizeof(int);
	shm->nr    = nrb*NR_RT_CPUS;
	shm->nc    = NCOLS;
	shm->nrb   = nrb;
	shm->nmeb  = NCOLS*shm->nrb;
	ofst = (ofst + sizeof(RTYPE) - 1)/sizeof(RTYPE);
	shm->slope_to_pos_mat_ofst = ofst;
	slope_to_pos_mat = (RTYPE *)shm + ofst;
	slopes = slope_to_pos_mat + shm->nmeb*NR_RT_CPUS;
	pos = slopes + NCOLS;

	init_mat(slope_to_pos_mat, slopes, nrb*NR_RT_CPUS, NCOLS);
	printf("<>>> RESULT CHECK: %d <<<>\n", 
			(int)(shm->nrb*RNUM*RNUM*NCOLS*(NCOLS + 1.0)/2.0));

	write(synfif, &shm, sizeof(shm));
	rtf_suspend_timed(synfif, DELAY);

	printf("NUMBER OF %d BYTES FILE SAVES: ", LOGFILE_SIZE);
	scanf("%d", &loops);

	if ((file = open(LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
		fprintf(stderr, "ERROR OPENING FILE %s\n", LOGFILE);
		exit(1);
	}

	while(loops--) {
		printf("HERE WE GO AGAIN\n");

		fsize = lost = 0;
		shm->run = 1;
		rtf_suspend_timed(synfif, DELAY);

		do {
			rtf_sem_wait(synfif);
			rtf_sem_wait(shmfif);
			if ((shm->frec + shm->nrecw - 1) > shm->maxrec) {
				nrecw = shm->maxrec - shm->frec + 1;
				write(file, buf_frec, nrecw*shm->recsize*sizeof(int));
				shm->frec = 1;
				buf_frec = buf_base;
				shm->nrecw -= nrecw;
				fsize += nrecw*recsize;
				if (*(buf_frec - shm->recsize) > fsize) { 
					lost += *(buf_frec - shm->recsize) - fsize; 
					printf("SIZE SAVED: %d, TRUE SIZE: %d, LOST: %d (INTs)\n", fsize, *(buf_frec - shm->recsize), lost);
					fsize = *(buf_frec - shm->recsize);
				}
			}
			if ((nrecw = shm->nrecw)) {
				write(file, buf_frec, nrecw*recsize*sizeof(int));
				shm->frec += nrecw;
				buf_frec += nrecw*recsize;
				shm->nrecw = -shm->nrecw;
				fsize += nrecw*recsize;
				if (*(buf_frec - shm->recsize) > fsize) { 
					lost += *(buf_frec - shm->recsize) - fsize; 
					printf("SIZE SAVED: %d, TRUE SIZE: %d, LOST: %d (INTs)\n", fsize, *(buf_frec - shm->recsize), lost);
					fsize = *(buf_frec - shm->recsize);
				}
			}
			rtf_sem_post(shmfif);
			if (rtf_read_timed(shmfif, &info, sizeof(info), DELAY)) {
				printf("CPU: %d, S: %4.1f, AJ: %d, MJ: %d, LOGCPU: %c, RES: %d, (%lld us)\n",
					info.cpuid, 
					info.cnt*(TICK_PERIOD/1.0E6), 
					info.avrj,
					info.maxj,
					info.last ? 'Y' : 'N',
					info.res,
					info.exctim);
			}
		} while (fsize < LOGFILE_SIZE/sizeof(int));

		shm->run = 0;
		lseek(file, 0, SEEK_SET);
		printf("END - FILE SIZE: %d, LOST: %d (CntDwn: %d)\n", fsize*sizeof(int), lost*sizeof(int), loops);
		rtf_suspend_timed(synfif, 500);
		lseek(shmfif, 0, SEEK_SET);
		shm->nrec = shm->nrecw = 0;
		shm->frec = shm->lrec  = 1;
		buf_frec = buf_lrec = buf_base;
	}

	close(file);
	rtai_free(nam2num(SHMNAM), shm);
	exit(0);
}
