use ExtUtils::MakeMaker;
WriteMakefile(
    'NAME'      	=> 'LXRT',
    'VERSION_FROM' 	=> 'LXRT.pm',
    'DEFINE'    	=> '',
    'INC'       	=> '-I/usr/src/linux/include -I.. -I../../include',   
    'PL_FILES'		=> { 'version.PL' => '../../include/version.h'}, 
    'depend'		=> { 'all:' => '../../include/version.h'},
);
