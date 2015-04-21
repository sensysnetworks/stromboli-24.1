/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

$Id: tabnewlxrt.c,v 1.1.1.1 2004/06/06 14:02:48 rpm Exp $ 
*/

/*
ACKNOWLEDGMENTS:
- Pierre Cloutier (pcloutier@poseidoncontrols.com) has suggested the 6 
  characters names and fixed many inconsistencies within this file.
*/


#include <rtai/version.h>
#include <rtai.h>
#include <rt_mem_mgr.h>
#if RTAI_VERSION_CODE < RTAI_VERSION(24,1,10)
#include "scheduler/rtai_sched.h"
#else
#include <rtai_sched.h>
#endif
#include <rtai_lxrt.h>

#include "msgnewlxrt.h"

static void nihil(void) { };

struct rt_fun_entry rt_fun_lxrt[] = {
	{ 1, rt_task_yield },				//   0  
	{ 1, rt_task_suspend },				//   1
	{ 1, rt_task_resume },				//   2
	{ 1, rt_task_make_periodic },			//   3
	{ 1, rt_task_wait_period },			//   4
	{ 1, rt_sleep },				//   5
	{ 1, rt_sleep_until },				//   6
	{ 0, start_rt_timer },				//   7
	{ 0, stop_rt_timer },				//   8
	{ 0, rt_get_time },				//   9
	{ 0, count2nano },				//  10
	{ 0, nano2count },				//  11
	{ 1, rt_sem_signal },				//  12
	{ 1, rt_sem_wait },				//  13
	{ 1, rt_sem_wait_if },				//  14
	{ 1, rt_sem_wait_until },			//  15
	{ 1, rt_sem_wait_timed },			//  16
	{ 0, rt_busy_sleep },				//  17
	{ 1, rt_send },					//  18
	{ 1, rt_send_if },				//  19
	{ 1, rt_send_until },				//  20
	{ 1, rt_send_timed },				//  21
	{ UW1(2, 0), rt_receive },			//  22
	{ UW1(2, 0), rt_receive_if },			//  23
	{ UW1(2, 0), rt_receive_until },		//  24
	{ UW1(2, 0), rt_receive_timed },		//  25
	{ UW1(3, 0), rt_rpc },				//  26
	{ UW1(3, 0), rt_rpc_if },			//  27
	{ UW1(3, 0), rt_rpc_until },			//  28
	{ UW1(3, 0), rt_rpc_timed },			//  29
	{ 0, rt_isrpc }, 		 		//  30
	{ 1, rt_return },				//  31
	{ 0, rt_set_periodic_mode },			//  32
	{ 0, rt_set_oneshot_mode },			//  33
	{ 0, rt_task_signal_handler },			//  34
	{ 0, rt_task_use_fpu },				//  35
	{ 0, rt_linux_use_fpu },			//  36
	{ 0, rt_preempt_always },			//  37
	{ 0, rt_get_time_ns },				//  38
	{ 0, rt_get_cpu_time_ns },			//  39
	{ UR1(2, 3), rt_mbx_send },			//  40
	{ UR1(2, 3), rt_mbx_send_wp },			//  41
	{ UR1(2, 3), rt_mbx_send_if },			//  42
	{ UR1(2, 3), rt_mbx_send_until },		//  43
	{ UR1(2, 3), rt_mbx_send_timed },		//  44
	{ UW1(2, 3), rt_mbx_receive },			//  45
	{ UW1(2, 3), rt_mbx_receive_wp },		//  46
	{ UW1(2, 3), rt_mbx_receive_if },		//  47
	{ UW1(2, 3), rt_mbx_receive_until },		//  48
	{ UW1(2, 3), rt_mbx_receive_timed },		//  49
	{ 0, rt_set_runnable_on_cpus },			//  50
	{ 0, rt_set_runnable_on_cpuid },		//  51
	{ 0, rt_get_timer_cpu },			//  52
	{ 0, start_rt_apic_timers },			//  53
	{ 0, rt_preempt_always_cpuid },			//  54
	{ 0, count2nano_cpuid },			//  55
	{ 0, nano2count_cpuid },			//  56
	{ 0, rt_get_time_cpuid },			//  57
	{ 0, rt_get_time_ns_cpuid },			//  58
// QNX IPC
	{ UR1(2, 4) | UW1(3, 5), rt_Send },		//  59
	{ UW1(2, 3) | UW2(4, 0), rt_Receive },		//  60
	{ UW1(2, 3) | UW2(4, 0), rt_Creceive },		//  61
	{ UR1(2, 3), rt_Reply },			//  62
	{ UR1(2, 3), rt_Proxy_attach },			//  63
	{ 1, rt_Proxy_detach },				//  64
	{ 1, rt_Trigger },				//  65
	{ 1, rt_Name_attach },				//  66
	{ 1, rt_Name_detach },				//  67
	{ 0, rt_Name_locate },				//  68
// qBlk's
	{ 0, nihil},					//  69
	{ 0, nihil},					//  70
	{ 0, nihil},					//  71
	{ 0, nihil},					//  72
	{ 0, nihil},					//  73
	{ 0, nihil},					//  74
	{ 0, nihil},					//  75
	{ 0, nihil},					//  76
	{ 0, nihil},					//  77
	{ 0, nihil},					//  78
	{ 0, nihil},					//  79
	{ 0, nihil},					//  80
	{ 0, nihil},					//  81
	{ 0, nihil},					//  82
	{ 0, nihil},					//  83
	{ 0, nihil},					//  84
	{ 0, nihil},					//  85
	{ 0, nihil},					//  86
	{ 0, nihil},					//  87
	{ 0, nihil},					//  88	
	{ 0, nihil},					//  89
	{ 0, nihil},					//  90
	{ 0, nihil},					//  91
	{ 0, nihil},					//  92
	{ 0, nihil},					//  93
	{ 0, nihil},					//  94
// Functions for testing in user space
	{ 0, nihil},					//  95
	{ 0, nihil},					//  96
	{ 0, nihil},					//  97
	{ 0, nihil},					//  98
	{ 0, nihil},	                 		//  99
// VC's
	{ 0, nihil},					// 100
	{ 0, nihil},					// 101
	{ 0, nihil},					// 102
// Linux Signal
	{ 0, nihil}, 			     		// 103
	{ 0, nihil},					// 104
	{ 0, nihil},			 		// 105
// Some more schedulers functions
	{ 1, rt_sem_broadcast },			// 106
	{ 1, rt_task_make_periodic_relative_ns },	// 107
	{ 0, rt_set_sched_policy },			// 108
	{ 1, rt_task_set_resume_end_times },		// 109
	{ 0, rt_spv_RMS },				// 110
	{ 0, rt_task_wakeup_sleeping },			// 111
// Extended intertask messages
	{ UR1(2, 4) | UW1(3, 5), rt_rpcx },		// 112
	{ UR1(2, 4) | UW1(3, 5), rt_rpcx_if },		// 113
	{ UR1(2, 4) | UW1(3, 5), rt_rpcx_until },	// 114
	{ UR1(2, 4) | UW1(3, 5), rt_rpcx_timed },	// 115
	{ UR1(2, 3), rt_sendx },			// 116
	{ UR1(2, 3), rt_sendx_if },			// 117
	{ UR1(2, 3), rt_sendx_until },			// 118
	{ UR1(2, 3), rt_sendx_timed },			// 119
	{ UR1(2, 3), rt_returnx },			// 120
	{ UW1(2, 3) | UW2(4, 0), rt_receivex },		// 121
	{ UW1(2, 3) | UW2(4, 0), rt_receivex_if },	// 122
	{ UW1(2, 3) | UW2(4, 0), rt_receivex_until },	// 123
	{ UW1(2, 3) | UW2(4, 0), rt_receivex_timed },	// 124
	{ UW1(2, 3) | UW2(4, 0), rt_evdrpx },		// 125
	{ UW1(2, 0), rt_evdrp },			// 126
	{ UW1(2, 3), rt_mbx_evdrp },			// 127
	{ 1, rt_change_prio },			 	// 128
	{ 0, rt_set_resume_time },  			// 129
	{ 0, rt_set_period },				// 130
	{ UR1(2, 3), rt_mbx_ovrwr_send },		// 131
	{ 0, rt_is_hard_timer_running },		// 132
	{ 1, rt_sem_wait_barrier },			// 133
	{ 1, rt_sem_count },				// 134
	{ 1, rt_cond_wait },				// 135
	{ 1, rt_cond_wait_until },                      // 136
	{ 1, rt_cond_wait_timed },			// 137
	{ 1, rt_rwl_rdlock },                           // 138
	{ 1, rt_rwl_rdlock_if },                        // 139
	{ 1, rt_rwl_rdlock_until },                     // 140
	{ 1, rt_rwl_rdlock_timed },                     // 141
	{ 1, rt_rwl_wrlock },                           // 142
	{ 1, rt_rwl_wrlock_if },                        // 143
	{ 1, rt_rwl_wrlock_until },                     // 144
	{ 1, rt_rwl_wrlock_timed },                     // 145
	{ 1, rt_rwl_unlock },                           // 146
	{ 1, rt_spl_lock },                             // 147
	{ 1, rt_spl_lock_if },                          // 148
	{ 1, rt_spl_lock_timed },                       // 149
	{ 1, rt_spl_unlock }                            // 150

};
