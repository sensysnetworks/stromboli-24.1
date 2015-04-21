
#ifndef _RTAI_RTAI_H_
#define _RTAI_RTAI_H_

#if !( __GNUC__ == 2 && __GNUC_MINOR__ > 8 && __GNUC_MINOR__ < 96 ) && \
	__GNUC__ != 3
#warning: You are likely using an unsuported GCC version! \
          Please read GCC-WARNINGS with care.
#endif

#include <asm/rtai.h>

#endif

