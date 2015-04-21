#!/usr/bin/perl -w
use LXRT;

$mtsk_name = nam2num("MTSK");
if( !( $mtsk = rt_task_init($mtsk_name, 0, 0, 0))) {
	die("can't set up scheduler $!\n");
	}
	
printf("MASTER TASK INIT: name = %x, address = %x\n", $mtsk_name, 
									$mtsk);

printf("MASTER TASK STARTS THE ONESHOT TIMER\n");
rt_set_oneshot_mode();
start_rt_timer(nano2count(1E7));

printf("MASTER TASK MAKES ITSELF PERIODIC WITH A PERIOD OF 1 ms\n");
rt_task_make_periodic($mtsk, rt_get_time(), nano2count(1E6));
rt_sleep(nano2count(1E9));

$count = 100; 
printf("MASTER TASK LOOPS ON WAIT_PERIOD FOR %d PERIODS\n", $count);
while($count--) {
	printf("PERIOD %d\n", $count);
	rt_task_wait_period();
	}

my $agent = rt_agent();
printf("MASTER TASK %X rt_agent() = %X\n",$mtsk, $agent);

printf("MASTER TASK STOPS THE PERIODIC TIMER\n");
stop_rt_timer();

printf("MASTER TASK DELETES ITSELF\n");
rt_task_delete($mtsk);

my $mtsk_name_string = "?";
num2nam($mtsk_name, $mtsk_name_string);
printf("END MASTER TASK [%s]\n", $mtsk_name_string);
