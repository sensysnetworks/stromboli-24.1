print "Auto generate ../../include/version.h\n";
open(VER, ">../../include/version.h");
print VER <<'TXT';
/* Auto generated from Perl LXRT bindings */
#define RTAI_VERSION_CODE 66048
TXT
close VER;

