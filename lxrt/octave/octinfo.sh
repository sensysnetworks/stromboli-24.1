#! /bin/sh

# Usage:
#   octinfo.sh [mkoctfile-ver]
#
# Produces one line of output containing:
#   octave-version m-path oct-path exec-path
# You can specify an alternative to mkoctfile as a parameter if for 
# example you have mkoctfile-2.0.16.91 and mkoctfile-2.1.31 both 
# installed on your system, each with its own set of header files 
# and libraries.
#
# This script works by compiling a .oct file using mkoctfile, with the
# appropriate #define constants stored in strings, then extracting
# those strings from the .oct file.  This will only check the info in
# include files; if your installation is screwy, the library and the 
# interpreter may have different version numbers. You can find the
# interpreter version number directly using:
#    octave -v | sed "s/^.*version \([0-9.]*\) .*$/\1/"

# Build the source with the embedded strings
cd /tmp
cat <<EOF >octaveinfo.cc
#include <octave/version.h>
#include <octave/defaults.h> 
#define INFOV "\nINFOV=" ## OCTAVE_VERSION ## "\n"

#ifdef OCTAVE_LOCALVERFCNFILEDIR
# define INFOM "\nINFOM=" ## OCTAVE_LOCALVERFCNFILEDIR ## "\n"
#else
# define INFOM "\nINFOM=" ## OCTAVE_LOCALFCNFILEPATH ## "\n"
#endif

#ifdef OCTAVE_LOCALVEROCTFILEDIR
# define INFOO "\nINFOO=" ## OCTAVE_LOCALVEROCTFILEDIR ## "\n"
#else
# define INFOO "\nINFOO=" ## OCTAVE_LOCALOCTFILEPATH ## "\n"
#endif

#ifdef OCTAVE_LOCALVERARCHLIBDIR
# define INFOX "\nINFOX=" ## OCTAVE_LOCALVERARCHLIBDIR ## "\n"
#else
# define INFOX "\nINFOX=" ## OCTAVE_LOCALARCHLIBDIR ## "\n"
#endif

char infom[] = INFOM;
char infoo[] = INFOO;
char infox[] = INFOX;
char infov[] = INFOV;
EOF

# Compile it (perhaps with a special version of mkoctfile)
if [ "$1" = "" ] ; then
    mkoctfile octaveinfo.cc || exit
else
    $1 octaveinfo.cc || exit
fi

eval `strings octaveinfo.oct | grep "^INFO.=" | sed -e "s/\/\/:.*$//"`
rm -rf octaveinfo.*
echo "$INFOV $INFOM $INFOO $INFOX " | sed -e 's,//*,/,g' -e 's,/ , ,g'
