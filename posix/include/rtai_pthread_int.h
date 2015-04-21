#ifndef _RTAI_PTHREAD_INT_H_
#define _RTAI_PTHREAD_INT_H_
//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
// Original date:       Thu 15 Jul 1999
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
// pthreads interface for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////

static char id_rtai_pthread_int_h[] __attribute__ ((unused)) = "@(#)$Id: rtai_pthread_int.h,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $";


#include <asm/signal.h>

#include <linux/sched.h>


// ----------------------------------------------------------------------------

#define POSIX_THREADS_MAX 64
#define PTHREAD_KEYS_MAX 1024

// Thread specific data is kept in a special data structure, a two-level
// array.  The top-level array contains pointers to dynamically allocated
// arrays of a certain number of data pointers.  So a sparse array can be
// implemented.  Each dynamic second-level array has
//      PTHREAD_KEY_2NDLEVEL_SIZE
// entries,and this value shouldn't be too large.
#define PTHREAD_KEY_2NDLEVEL_SIZE       32

// Need to address PTHREAD_KEYS_MAX key with PTHREAD_KEY_2NDLEVEL_SIZE
// keys in each subarray.
#define PTHREAD_KEY_1STLEVEL_SIZE \
  ((PTHREAD_KEYS_MAX + PTHREAD_KEY_2NDLEVEL_SIZE - 1) \
  / PTHREAD_KEY_2NDLEVEL_SIZE)


#define TASK_FPU_DISABLE 0
#define TASK_FPU_ENABLE 1

#define NSECS_PER_SEC 1000000000


// ----------------------------------------------------------------------------


// Arguments passed to thread creation routine.

struct pthread_start_args {
  void * (*start_routine)(void *); // function to run
  void * arg;                   // its argument
  sigset_t mask;                // initial signal mask for thread
  int schedpolicy;              // initial scheduling policy (if any)
  struct sched_param schedparam; // initial scheduling parameters (if any)
};


#define PTHREAD_START_ARGS_INITIALIZER { NULL, NULL, 0, 0, { 0 } }


struct _pthread_descr_struct {
  pthread_descr p_nextlive, p_prevlive;
                                // Double chaining of active threads
  pthread_descr p_nextwaiting;  // Next element in the queue holding the thr
  pthread_t p_tid;              // Thread identifier
  int p_pid;                    // PID of Unix process
  int p_priority;               // Thread priority (== 0 if not realtime)
  int p_policy;                 // Thread policy (== SCHED_OTHER if not realtime)
  int *p_spinlock;              // Spinlock for synchronized accesses
  int p_signal;                 // last signal received
  rt_jmp_buf *p_signal_jmp;     // where to siglongjmp on a signal or NULL
  rt_jmp_buf *p_cancel_jmp;     // where to siglongjmp on a cancel or NULL
  char p_terminated;            // true if terminated e.g. by pthread_exit
  char p_detached;              // true if detached
  char p_exited;                // true if the assoc. process terminated
  void *p_retval;               // placeholder for return value
  int p_retcode;                // placeholder for return code
  pthread_descr p_joining;      // thread joining on that thread or NULL
  struct _pthread_cleanup_buffer *p_cleanup; // cleanup functions
  char p_cancelstate;           // cancellation state
  char p_canceltype;            // cancellation type (deferred/async)
  char p_canceled;              // cancellation request pending
  int p_errno;                  // error returned by last system call
  int p_h_errno;                // error returned by last netdb function
  struct pthread_start_args p_start_args; // arguments for thread creation
  void **p_specific[PTHREAD_KEY_1STLEVEL_SIZE]; // thread-specific data
  RT_TASK rtask_struct;         // RTAI task structure.
};



// ----------------------------------------------------------------------------


static inline void ts_from_ns (long long t, struct timespec *t_spec)
{
	t_spec->tv_sec = ulldiv(t, NSECS_PER_SEC, (unsigned long *)&t_spec->tv_nsec);
	if (t_spec->tv_nsec < 0) {
		t_spec->tv_nsec += NSECS_PER_SEC;
		t_spec->tv_sec --;
	}
}


#endif  // _RTAI_PTHREAD_INT_H_
