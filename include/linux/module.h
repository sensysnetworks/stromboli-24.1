/*
 * module.h compatibility header
 */

#ifndef _COMPAT_MODULE_H
#define _COMPAT_MODULE_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < 0x020115
#define MODULE_AUTHOR(a)
#define MODULE_DESCRIPTION(a)
#define MODULE_PARM(a,b)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10)
#define MODULE_LICENSE(x)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18)		/* ? */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
  #ifdef MODULE
    /* don't ask.  It works. */
    #define THIS_MODULE			((struct module *)&mod_use_count_)
  #else
    #define THIS_MODULE	NULL
  #endif
  #define __MOD_INC_USE_COUNT(x)	((*(long *)(x))++, (*(long *)(x)) |= MOD_VISITED)
  #define __MOD_DEC_USE_COUNT(x)	((*(long *)(x))--, (*(long *)(x)) |= MOD_VISITED)
  #define EXPORT_SYMBOL(x)
#else
  #ifdef MODULE
    #define THIS_MODULE	&__this_module
  #else
    #define THIS_MODULE	NULL
  #endif
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define MODULE_DEVICE_TABLE(a,b)
#endif

#include_next <linux/module.h>

#endif /* _COMPAT_MODULE_H */




