Options for 'make'
==================

The Makefile has the following options available:

make           - This will build the RTAI module only.
make all       - If you have compiled the kernel with SMP option, this will
		 build the smp and mup schedulers, the fifo modules, the
		 calibration modules and all of the examples;
		 if you have compiled the kernel with UP option, this will
		 build the up scheduler, the fifo modules, the calibration
		 modules and the examples.
		 (see also Rules.make)
make clean     - This will clean the rtai module.
make cleanall  - This will clean the scheduler modules, fifo module,
                 the calibration modules and the examples.
make distclean - Simply clean everything. Afterwards the source tree 
		 should be in the state it was when freshly extracted.

Brendan Knox (brendank@zentropix.com)
Lorenzo Dozio (dozio@aero.polimi.it)
