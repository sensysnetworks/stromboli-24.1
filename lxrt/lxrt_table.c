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

$Id: lxrt_table.c,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $ 
*/

/*
ACKNOWLEDGMENTS:
- Pierre Cloutier (pcloutier@poseidoncontrols.com) has suggested the 6 
  characters names and fixed many inconsistencies within this file.
*/

#include <rtai.h>
#include <rt_mem_mgr.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>

#include "msg.h"
#include "qblk.h"
#include "names.h"
#include "rtai_signal.h"

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
	{ 0, rt_InitTickQueue},				//  69
	{ 0, rt_ReleaseTickQueue},			//  70
	{ 0, rt_qDynAlloc},				//  71
	{ 0, rt_qDynFree},				//  72
	{ 0, rt_qDynInit},				//  73
	{ 1, rt_qBlkWait},				//  74
	{ 1, rt_qBlkRepeat},				//  75
	{ 1, rt_qBlkSoon},				//  76
	{ 1, rt_qBlkDequeue},				//  77
	{ 1, rt_qBlkCancel},				//  78
	{ 1, rt_qSync},					//  79
	{ UW1(2, 3) | UW2(4, 0), rt_qReceive},		//  80
	{ 1, rt_qLoop},					//  81
	{ 1, rt_qStep},					//  82
	{ 1, rt_qBlkBefore},				//  83
	{ 1, rt_qBlkAfter},				//  84
	{ 1, rt_qBlkUnhook},				//  85
	{ 1, rt_qBlkRelease},				//  86
	{ 1, rt_qBlkComplete},				//  87
	{ 1, rt_qHookFlush},				//  88	
	{ 0, rt_qBlkAtHead},				//  89
	{ 0, rt_qBlkAtTail},				//  90
	{ 0, rt_qHookInit},				//  91
	{ 0, rt_qHookRelease},				//  92
	{ 1, rt_qBlkSchedule},				//  93
	{ 0, rt_GetTickQueueHook},			//  94
// Functions for testing in user space
	{ 1, rt_boom},					//  95
	{ 0, rt_malloc},				//  96
	{ 0, rt_free},					//  97
	{ 0, rt_mmgr_stats},				//  98
	{ 0, rt_stomp},                 		//  99
// VC's
	{ 1, rt_vc_attach},				// 100
	{ 0, rt_vc_release},				// 101
	{ 0, rt_vc_reserve},				// 102
// Linux Signal
	{ 0, rt_get_linux_signal},      		// 103
	{ 0, rt_get_errno},				// 104
	{ 0, rt_set_linux_signal_handler}, 		// 105
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
        { 1, rt_sem_wait_barrier }			// 133
};
