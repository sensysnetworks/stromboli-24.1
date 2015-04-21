package LXRT;

use strict;
use vars qw($VERSION @ISA @EXPORT);

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(	rtai_lxrt nam2num num2nam rt_get_adr rt_task_init 
		rt_task_delete rt_task_yield rt_task_suspend 
		rt_task_resume rt_task_make_periodic rt_task_wait_period
		rt_sleep rt_sleep_until start_rt_timer stop_rt_timer
		rt_get_time count2nano nano2count rt_busy_sleep rt_send
		rt_send_if rt_send_until rt_send_timed rt_receive
		rt_receive_if rt_receive_until rt_receive_timed
		rt_set_oneshot_mode
		rt_sem_init rt_sem_delete rt_sem_signal rt_sem_wait rt_sem_wait_if
		rt_sem_wait_until rt_sem_wait_timed rt_agent
		);
$VERSION = '0.30';

BEGIN {
my $ncpus     = `cat /proc/cpuinfo | grep 'cpu MHz'| wc -l`;
my $scheduler = $ncpus > 1 ? "smpscheduler/rtai_sched_apic"
			: "upscheduler/rtai_sched";

#
# NOTE: I comment this out until we install modules where modprobe expects to find them. PGGC.
#
#system("modprobe rtai");
#system("modprobe rtai_sched");
#system("modprobe lxrt");

#Paranoid ?
system("sync");
}

bootstrap LXRT $VERSION;

# Preloaded methods go here.

