
#ifndef _COMPAT_VERSION_H
#define _COMPAT_VERSION_H

#include_next <linux/version.h>

/* LINUX_TESTVERSION is for that pesky -testX subversioning
 * that is so annoying */
#ifndef LINUX_TESTVERSION
#define LINUX_TESTVERSION 0xff
#endif

/* And, since we really want it part of LINUX_VERSION_CODE... */
#define LINUX_EXT_VERSION_CODE ((LINUX_VERSION_CODE << 8) | LINUX_TESTVERSION)

#define KERNEL_EXT_VERSION(a,b,c,d) ((KERNEL_VERSION(a,b,c)<<8) | d)

#endif

