

#include <stdio.h>
#include <leds_lxrt.h>

int main(int argc, char *argv[])
{
	int i, j;

	rt_reset_leds(255);

	for( i=0 ; i < 256 ; i++ ) {
		rt_set_leds(i);
		for(j=0;j<1000000;j++) j=j;
	}

	return 0;
}


