/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: test.cc,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
 *
 * COPYRIGHT: (C) 2001  Paolo Mantegazza <mantegazza@aero.polimi.it>
 *                2002  Robert Schwebel  <robert@schwebel.de>
 *                2002  Erwin Rol <erwin@muffin.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ACKNOWLEDGMENT: 
 * 
 * Part of this code is derived from that of the latency calibration example,
 * which in turn was copied from a similar program distributed with NMT-RTL.
 */

#include "module.h"
#include "modinfo.h"
#include "time.h"
#include "count.h"
#include "task.h"
#include "rtf.h"
#include "iostream.h"
#include "trace.h"
#include "linux_wrapper.h"
#include "rtai_wrapper.h"

static int loops;

#define DEBUG_FIFO 3
#define TIMER_TO_CPU 3		// < 0  || > 1 to maintain a symmetric processed timer.
#define RUNNABLE_ON_CPUS 3	// 1: on cpu 0 only, 2: on cpu 1 only, 3: on any;
#define RUN_ON_CPUS ( __smp_num_cpus() > 1 ? RUNNABLE_ON_CPUS : 1)

/* 
 *	/proc/rtai/latency_calibrate entry
 */

/*
#ifdef CONFIG_PROC_FS
static int proc_read(char *page, char **start, off_t off, 
                     int count, int *eof, void *data)
{
	PROC_PRINT_VARS;
	PROC_PRINT("## RTAI latency calibration tool \n");
	PROC_PRINT("# overall=%i\n",overall);
	PROC_PRINT("# period=%i\n",period);
	PROC_PRINT("# avrgtime=%i\n",avrgtime);
	PROC_PRINT("# use_fpu=%i\n",use_fpu);
	PROC_PRINT("# start_timer=%i\n",start_timer);
	PROC_PRINT("\n");
	PROC_PRINT_DONE;
}
#endif
*/

static RTAI::TraceEvent* g_TraceEvent;

struct sample 
{ 
	long long min; 
	long long max; 
	int index; 
};

class Thread 
:	public RTAI::Task 
{
public:
	Thread(int stack, bool use_fpu)
	:	RTAI::Task(stack,0,use_fpu,0,0)
	{
		m_fifo = new RTAI::fifoT<sample>(DEBUG_FIFO, 16000);
	}

	~Thread()
	{
		delete m_fifo;
	}

	int make_periodic( const RTAI::Count& start, const RTAI::Count& period)
	{
		m_Period = period;
		m_Expected = start;

		return RTAI::Task::make_periodic( start, period );
	}

	int run()
	{
		sample samp;
		int diff = 0;
		int average;
		int min_diff = 0;
		int max_diff = 0;

		// If we want to make overall statistics
		// we have to reset min/max here
		if( overall ) 
		{
			min_diff =  1000000000;
			max_diff = -1000000000;
		}
		
		while(1) 
		{
			// Not overall statistics: reset min/max
			if (!overall) {
				min_diff =  1000000000;
				max_diff = -1000000000;
			}

			average = 0;

			for( int i = 0; i < loops; i++ ) 
			{
				inc_cpu_use();

				m_Expected += m_Period;
				wait_period();

				diff = (RTAI::Count::now() - m_Expected).to_time();

				if (diff < min_diff) { min_diff = diff; }
				if (diff > max_diff) { max_diff = diff; }
				average += diff;

				g_TraceEvent->trigger(sizeof (diff), &diff);
			}
			samp.min = min_diff;
			samp.max = max_diff;
			samp.index = average / loops;

			m_fifo->put( &samp );
		}

		return -1;
	}
protected:
	RTAI::Count m_Period;
	RTAI::Count m_Expected;
	RTAI::fifoT<sample>* m_fifo;
};


class Module
:	public RTAI::Module
{
public:
	Module()
	{
		// Initialisation. We have to select the scheduling mode and start 
		// our periodical measurement task.  
		RTAI::Count start, period_counts;

		/* XXX check option ranges here */

		/* register a proc entry */
#ifdef CONFIG_PROC_FS
//		create_proc_read_entry("rtai/latency_calibrate", /* name             */
//	                       0,			 /* default mode     */
//	                       NULL, 			 /* parent dir       */
//			       proc_read, 		 /* function         */
//			       NULL			 /* client data      */
//		);
#endif
		g_TraceEvent = new RTAI::TraceEvent( "latency", 0, 0 );

		RTAI::linux_use_fpu(use_fpu);

		m_Thread = new Thread( 16*1024, use_fpu );
		m_Thread->set_runnable_on_cpus(RUN_ON_CPUS);

		// Test if we have to start the timer
		if (start_timer) {
			RTAI::set_oneshot_mode();
			RTAI::assign_irq_to_cpu(RTAI::TIMER_8254_IRQ, TIMER_TO_CPU);
			period_counts = RTAI::start_timer(RTAI::Count::from_time(period) );
		} else {
			period_counts = RTAI::Count::from_time( period );
		}

		loops = (1000000000*avrgtime)/period;

		// Calculate the start time for the task.
		// We set this to "now plus 10 peroids"   

		start = RTAI::Count::now() + ( 10 * period_counts );

		m_Thread->make_periodic( start, period_counts );
	}

	~Module()
	{
		// Cleanup 

		// If we started the timer we have to revert this now.
		if (start_timer) {
			RTAI::reset_irq_to_sym_mode(RTAI::TIMER_8254_IRQ);
			RTAI::stop_timer();
		}

		// Output some statistics about CPU usage 
		m_Thread->dump_cpu_use();

		// Now delete our task and remove the FIFO.
		delete m_Thread;
		delete g_TraceEvent;
		// Remove proc dir entry 
#ifdef CONFIG_PROC_FS
//		remove_proc_entry("rtai/latency_calibrate", NULL);
#endif
	}
protected:
	Thread *m_Thread;
};

Module theModule;
