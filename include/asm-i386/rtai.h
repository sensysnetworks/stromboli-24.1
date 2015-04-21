#include <config.h>
#ifdef CONFIG_RTAI_LINUX22
#include <asm/rtai-22.h>
#elif CONFIG_RTAI_ADEOS
#include <asm/arti.h>
#else
#include <asm/rtai-24.h>
#endif
