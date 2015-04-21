/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)
m68knommu contributed by Lineo Inc. (Author: Bernhard Kuhn (bkuhn@lineo.com)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/config.h>

#include <asm/system.h>
#include <asm/hw_irq.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/atomic.h>
#include <asm/machdep.h>
#include <asm/hardirq.h>
#include <asm/traps.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include "rtai_proc_fs.h"
#endif

#include <asm/rtai.h>
#include <asm/rtai_srq.h>


struct {
  int pending_srqs;
  int active_srqs;
  int pending_irqs;
  int active_irqs;
  int cpu_in_sti;
  int used_by_linux;
  int locked_cpus;
  int current_priority;
} global = {0,0,0,0,0,1,0,-1};



/* --------------< set and reset exception vectors >-------------- */

extern e_vector *_ramvec;

struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type, int dpl, void (*handler)(void)) {
// "dpl" is the descriptor privilege level: 0-highest, 3-lowest.
// "type" is the interrupt type: 14 interrupt (cli), 15 trap (no cli).
// for m68k, we just ignore dpl and type for now!
  struct desc_struct* vector_table=(struct desc_struct*)_ramvec;
  struct desc_struct idt_element = vector_table[vector];
  vector_table[vector].a=handler;
  return idt_element;
};

void rt_reset_full_intr_vect(unsigned int vector, struct desc_struct idt_element) {
  struct desc_struct* vector_table=(struct desc_struct*)_ramvec;
  vector_table[vector] = idt_element;
  return;
}



/* -------------------< system request handling >------------------ */

/*
  The number of rtai system requests is limited
  by the number of bits in an unsigned integer,
  representing the pending requests.
*/

#define RT_NUM_SRQS 32

static struct sysrq_t {
        unsigned int label;
        void (*rtai_handler)(void);
        long long (*user_handler)(unsigned int whatever);
} sysrq[RT_NUM_SRQS];

static void rt_srq_init(void) {
  int i=0;
  for(i=0;i<RT_NUM_SRQS;i++) {
    sysrq[i].label=0;
    sysrq[i].rtai_handler=0;
    sysrq[i].user_handler=0;
  };
};

int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever)) {
  unsigned long flags;
  int srq;

  hard_save_flags_and_cli(flags);
  if (!rtai_handler) {
    hard_restore_flags(flags);
    return -EINVAL;
  }
  for (srq = 2; srq < RT_NUM_SRQS; srq++) {
    if (!(sysrq[srq].rtai_handler)) {
      sysrq[srq].label = label;
      sysrq[srq].rtai_handler = rtai_handler;
      sysrq[srq].user_handler = user_handler;
      hard_restore_flags(flags);
      return srq;
    }
  }
  hard_restore_flags(flags);
  return -EBUSY;
};

int rt_free_srq(unsigned int srq) {
  unsigned long flags;
  
  hard_save_flags_and_cli(flags);
  if (srq < 2 || srq >= RT_NUM_SRQS || !sysrq[srq].rtai_handler) {
    hard_restore_flags(flags);
    return -EINVAL;
  }
  sysrq[srq].rtai_handler = 0; 
  sysrq[srq].user_handler = 0; 
  sysrq[srq].label = 0;
  hard_restore_flags(flags);
  return 0;
};

void rt_pend_linux_srq(unsigned int srq) {
  set_bit(srq, &global.pending_srqs);
};

/*
  user space rtai system requests will use the current application
  stack pointer ... no need to switch over to kernel stack!?
*/

static void srqisr(void) {
  __asm__ __volatile__ ("movew %0,%%sr\n\t"
			"lea %%sp@(-36),%%sp\n\t"
			"moveml %%d0-%%d5/%%a0-%%a2,%%sp@\n\t"
			"jsr dispatch_srq\n\t"
			"addql #8,%%sp\n\t"
			"moveml %%sp@,%%d2-%%d5/%%a0-%%a2\n\t"
			"lea %%sp@(28),%%sp\n\t"
			"rte\n\t" : : "i" (LINUX_IRQS_DISABLE));
}

long long dispatch_srq(unsigned int srq, unsigned int whatever)
{
        if (srq > 1 && srq < RT_NUM_SRQS && sysrq[srq].user_handler) {
                return sysrq[srq].user_handler(whatever);
        }
        for (srq = 2; srq < RT_NUM_SRQS; srq++) {
                if (sysrq[srq].label == whatever) {
                        return (long long)srq;
                }
        }
        return 0;
}



/* -----------------< pending linux interrupt handling >----------------- */

/*
  It doesn't make sense to have a 256 bit wide
  register for indicating pending linux interrupts.
  Instead, each interrupt number (0-255) is mapped to
  a mask bit (0-31) in an unsigned integer and
  vice verca, limiting the number of pendable
  interrupts to 32 ... this should be sufficient
  for most applications :-) (bkuhn@lineo.com)
 */

#define LX_NUM_PEND_IRQS (32)

static int lx_pend_irq[LX_NUM_PEND_IRQS];
static int lx_pend_mask[NR_IRQS];
static int lx_pend_inuse[NR_IRQS];

extern irq_node_t *int_irq_list[NR_IRQS];
static unsigned long irq_action_flags[NR_IRQS];
static int chained_to_linux[NR_IRQS];
static int rtai_irq_inuse[NR_IRQS];

static void lx_irq_init(void) {
  int i;
  for(i=0;i<LX_NUM_PEND_IRQS;i++) lx_pend_irq[i]=-1;  
  for(i=0;i<NR_IRQS;i++) {
    lx_pend_mask[i]=-1;
    lx_pend_inuse[i]=0;
    chained_to_linux[i]=0;
  };  
};

static int lx_irq_findfree(void) {
  int i;
  for(i=0;i<LX_NUM_PEND_IRQS;i++) if(lx_pend_irq[i]==-1) return i;
  return -1;
};

static void lx_irq_pend_inuse_inc(unsigned int irq) {
  unsigned long flags;
  hard_save_flags_and_cli(flags);
  if (++rtai_irq_inuse[irq]==1) {
    int mask=lx_irq_findfree();
    lx_pend_irq[mask] = irq;
    lx_pend_mask[irq] = mask;
  }
  hard_restore_flags(flags);
};

static void lx_irq_pend_inuse_dec(unsigned int irq) {
  unsigned long flags;
  hard_save_flags_and_cli(flags);
  if ((--rtai_irq_inuse[irq])==0) {
    int mask=lx_pend_mask[irq];
    lx_pend_irq[mask] = -1;
  }
  hard_restore_flags(flags);
};

int rt_request_linux_irq(unsigned int irq,
			 void (*linux_handler)
			 (int irq, void *dev_id,
			  struct pt_regs *regs),
			 char *linux_handler_id, void *dev_id) {
  int flags;

  if (irq >= NR_IRQS || !linux_handler) return -EINVAL;

  hard_save_flags_and_cli(flags);
  lx_irq_pend_inuse_inc(irq);
  chained_to_linux[irq]++;
  if (chained_to_linux[irq]==1) {
    if (int_irq_list[irq]) {
      irq_action_flags[irq] = int_irq_list[irq]->flags;
      int_irq_list[irq]->flags |= SA_SHIRQ;
    }
  }
  hard_restore_flags(flags);

  request_irq(irq, linux_handler, SA_SHIRQ, linux_handler_id, dev_id);
  
  return 0;
};

int rt_free_linux_irq(unsigned int irq, void *dev_id) {
  int flags;

  if (irq >= NR_IRQS || !chained_to_linux[irq]) return -EINVAL;

  free_irq(irq, dev_id);

  hard_save_flags_and_cli(flags);
  if (!(--chained_to_linux[irq])) {
    if(int_irq_list[irq]) {
      int_irq_list[irq]->flags = irq_action_flags[irq];
    };
  }
  lx_irq_pend_inuse_dec(irq);
  hard_restore_flags(flags);

  return 0;
};

void rt_pend_linux_irq(unsigned int irq) {
  set_bit(lx_pend_mask[irq], &global.pending_irqs);
};

static void linux_irq_check(void) {

  extern void process_int(int vec,void*);
  extern unsigned int *mach_kstat_irqs;

  while(!test_and_set_bit(0, &global.cpu_in_sti)) {
    
    while( global.pending_irqs || global.pending_srqs ) {
      
      int srq,irq;

      hard_cli();
      if ((srq = global.pending_srqs & ~global.active_srqs)) {
        srq = ffnz(srq);
        set_bit(srq, &global.active_srqs);
        clear_bit(srq, &global.pending_srqs);
        soft_cli();
        if (sysrq[srq].rtai_handler) {
          sysrq[srq].rtai_handler();
        }
        clear_bit(srq, &global.active_srqs);
      }
      else {
        soft_cli();
      }

      hard_cli();
      if ((irq = global.pending_irqs & ~global.active_irqs)) {
        irq = ffnz(irq);
        set_bit(irq, &global.active_irqs);
        clear_bit(irq, &global.pending_irqs);
        soft_cli();

	mach_kstat_irqs[lx_pend_irq[irq]]++;
	process_int(lx_pend_irq[irq],0);

        clear_bit(irq, &global.active_irqs);
      }
      else {
        soft_cli();
      }
      
    };

    clear_bit(0, &global.cpu_in_sti);
    if (!(global.pending_irqs | global.pending_srqs)) break;
  };

};



/* ------------------------< rtai rthal >------------------- */

static void linux_cli(void) {
  soft_cli();
};

static void linux_sti(void) {
  if(global.pending_irqs || global.pending_srqs) {
    soft_cli();
    linux_irq_check();
  };
  soft_sti();
};

static unsigned int linux_save_flags(void) {
  int x;
  hard_save_flags(x);
  return x;
}

static void linux_restore_flags(unsigned int x) {
  if(!(x&0x700)) {
    if(global.pending_irqs || global.pending_srqs) {
      soft_cli();
      linux_irq_check();
    };
    soft_sti();
  }
  else {
    soft_cli();
  };
};

static unsigned int linux_save_flags_and_cli(void) {
  int x;
  hard_save_flags(x);
  soft_cli();
  return x;
};

unsigned int linux_save_flags_and_cli_cpuid(int cpuid) {
  return linux_save_flags_and_cli();
};

void rtai_just_copy_back(unsigned long flags, int cpuid) {
  linux_restore_flags(flags);
};

struct rt_hal linux_rthal;

struct rt_hal rtai_rthal = {
  0, // &ret_from_intr,
  0, // __switch_to,
  0, // idt_table,
  linux_cli,
  linux_sti,
  linux_save_flags,
  linux_restore_flags,
  linux_save_flags_and_cli,
  0, // irq_desc,
  0, // irq_vector,
  0, // irq_affinity,
  0, // smp_invalidate_interrupt,
  0, // ack_8259_irq,
  0, // &idle_weight,
  0,
  0, // switch_mem,
  0, // init_tasks
};



/* -----------------------< rtai irq handling >---------------- */

/*
  special hack: at the beginning of an interrupt
  service routine, we need to save the current content
  of the status register for later use, but on the
  other hand we need to disable interrupts at the
  beginning of the isr to protect the subsequent
  code region (potential switch from user stack pointer
  to kernel stack pointer) from re-entering.

  The second condition can be meet be starting
  the isr with "move #0x2700,%sr", but this will
  obviously destroy the content of the status register
  (making it impossible to meet the first condition).

  Now, the trick is to do a "trap #10" instead (at the
  beginning of the isr) causing a subsequent
  execption that disables the interrupts be doing a "move #0x2700,%sr"
  as its first instruction (remember: the first instruction
  of any exception will be executed under any circumstance).
  This certainly will also destroy the content of the
  status register, but it has been previsously saved on the
  stack by the exception (!). Means: we can now access
  the content of the status register as it was when entering
  the isr and save it to memory (sr_saved). To avoid
  re-enabling the interrupts by the subsequent "rte",
  because rte will load the content of the status
  register stored on the stack,we will have to modify its
  content directly on the stack. (bkuhn@lineo.com)
 */

#define SAVE_SR_TRAP __asm__ __volatile__ ( \
	"movew	#0x2700,%sr  ;" \
        "movel  %d0,%sp@-    ;" \
        "movew  %sp@(6),%d0  ;" \
        "movew  %d0,sr_saved ;" \
        "movew  #0x2700,%d0  ;" \
        "movew  %d0,%sp@(6)  ;" \
        "movel  %sp@+,%d0    ;" \
        "rte                 ;" )

unsigned short sr_saved;

static void save_sr_trap(void) {
  SAVE_SR_TRAP;
};

/*
  We are saving registers on the stack just like the standard
  linux kernel would do it. This makes it easier in
  case we have to jump to ret_from_interrupt due to
  pended interrupts. Also, we have to switch
  to kernel stack, if necessary ...
 */

#define SAVE_REG __asm__ __volatile__ ( \
	"trap   #10      	;" \
	"btst   #5,%sp@(2)	;" \
	"bnes	1f		;" \
	"			;" \
	"link	%a0,#12 	;" \
	"movel	%sp,sw_usp	;" \
	"movel	sw_ksp,%sp	;" \
	"movel	%a0@(8),%sp@-	;" \
	"movel	%a0@(4),%sp@-	;" \
	"movel	%a0@,%a0	;" \
"1:				;" \
	"clrl	%sp@-		;" \
        "pea    -1:w            ;" \
	"movel	%d0,%sp@-	;" \
        "movew  sr_saved,%d0    ;" \
	"movew	%d0,%sr         ;" \
	"lea	%sp@(-32),%sp	;" \
	"moveml	%d1-%d5/%a0-%a2,%sp@;" \
        "                       ; "\
        "movel   %sp@(44),%d0   ; "\
        "movel   %sp,%sp@-      ; "\
        "movel   %d0,%sp@-      ; "\
        "swap    %d0            ; "\
        "andl    #0x03fc,%d0    ; "\
        "lsrl    #2,%d0         ; "\
        "movel   %d0,%sp@-      ; ")

#define RSTR_REG __asm__ __volatile__ ( \
        "lea    %sp@(12),%sp    ;" \
        "tstl   %d0             ;" \
        "bnes   2f              ;" \
        "                       ;" \
	"moveml	%sp@,%d1-%d5/%a0-%a2;" \
	"lea	%sp@(32),%sp   	;" \
	"movel	%sp@+,%d0	;" \
	"addql	#4,%sp		;" \
	"addl	%sp@+,%sp	;" \
	"			;" \
	"move	#0x2700,%sr	;" \
	"btst	#5,%sp@(2)	;" \
	"bnes	1f		;" \
	"			;" \
	"link	%a0,#12		;" \
	"movel	%sp,sw_ksp	;" \
	"movel	sw_usp,%sp	;" \
	"movel	%a0@(8),%sp@-	;" \
	"movel	%a0@(4),%sp@-	;" \
	"movel	%a0@,%a0	;" \
"1:				;" \
	"rte			;" \
"2:                             ;" \
        "addql  #1,local_irq_count;" \
        "jmp ret_from_interrupt ;" )

static void rt_rtai_irqvec(void) {
        SAVE_REG;
        __asm__ __volatile__ ("jsr rt_dispatch_irqvec\n\t");
        RSTR_REG;
}

static struct rt_irq_t {
  void (*handler)(void);
  unsigned int enabled;
  unsigned int flags;
  void (*ack)(void);
} rt_irq[NR_IRQS];

struct rt_regs {
  unsigned long d1,d2,d3,d4,d5,a0,a1,a2,d0,orig_d0,stack_adj;
  unsigned short vec;  
  unsigned short sr;
  unsigned long pc;
};

int rt_dispatch_irqvec(int vec,int sr,struct rt_regs* regs) {

  if(rt_irq[vec].enabled) {
    if(rt_irq[vec].ack) rt_irq[vec].ack();
    rt_irq[vec].handler();
  };

  if(global.pending_irqs || global.pending_srqs) {
    if(!(sr & 0x0700)) {
      soft_cli();
      linux_irq_check();
      return 1;
    };
  };
  
  return 0;

};

static void rt_irq_init(void) {
  int i;
  for(i=0;i<NR_IRQS;i++) {
    rt_irq[i].handler=0;
    rt_irq[i].enabled=0;
    rt_irq[i].flags=0;
    rt_irq[i].ack=0;
  };
};

/*
  Actualy, we don't have 256 interrupts, but assuming
  that makes live much easier :-) . In fact,
  there are 7 non-autovector interrupts (25 = level 1
  through 31 = level 7; BTW.: 31 is non-maskible)
  and 192 autovector interrupts (64 through 255);
*/

struct desc_struct rt_linux_irqvec[NR_IRQS];

int rt_request_global_irq(unsigned int i,void (*handler)(void)) {
  if(i>=NR_IRQS) return -1;
  else {
    struct rt_irq_t *r=&rt_irq[i];
    if(r->enabled) return -1;
    else {
      r->handler=handler;
      r->enabled=1;
      rt_linux_irqvec[i] = rt_set_full_intr_vect(i,0,0,rt_rtai_irqvec);
      lx_irq_pend_inuse_inc(i);
    };
  };
  return 0;
};

int rt_free_global_irq(unsigned int i) {
  if(i>=NR_IRQS) return -1;
  else {
    struct rt_irq_t *r=&rt_irq[i];
    if(!r->enabled) return -1;
    else {
      rt_reset_full_intr_vect(i,rt_linux_irqvec[i]);
      r->handler=0;
      r->enabled=0;
      lx_irq_pend_inuse_dec(i);
    };
  };
  return 0;
};

void rt_enable_irq(unsigned int irq) {};
void rt_disable_irq(unsigned int irq) {};
RT_TRAP_HANDLER rt_set_rtai_trap_handler(RT_TRAP_HANDLER handler) {
  return 0;
};

/* ---------------------< timer functions >------------------- */

#include <asm/coldfire.h>
#include <asm/mcftimer.h>
#include <asm/mcfsim.h>

struct calibration_data tuned;

struct rt_times rt_times;

void rt_set_timer_delay(unsigned short delay) {
  volatile unsigned short *timerp;
  timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE4);

  if(delay) {
    timerp[MCFTIMER_TRR] = rdtsc() + delay;
  }
#ifndef CONFIG_RTAI_TIMER_TRUE_PERIODIC
  else {
    timerp[MCFTIMER_TRR] = rt_times.intr_time;
  };
#endif
};

static void rt_ack_tmr(void) {
  volatile unsigned char  *timerp;
  
  /* Reset the ColdFire timer */
  timerp = (volatile unsigned char *) (MCF_MBAR + MCFTIMER_BASE4);
  timerp[MCFTIMER_TER] = MCFTIMER_TER_CAP | MCFTIMER_TER_REF;

  /* keep emulated time stamp counter up to date */
#ifndef CONFIG_RTAI_TIMER_TRUE_PERIODIC
  rdtsc();
#endif
};

#ifdef CONFIG_NETtel
#define RT_TIMER_IRQ 30
#endif
#ifdef CONFIG_MOTOROLA
#define RT_TIMER_IRQ 72
#endif

static void (*lx_mach_tick)(void)=0;

/*
  When requesting the timer, it is necessary
  to assign a corresponding bit for global.pending_irqs
  (by lx_irq_chain()) so that it is possible to pend an irq
  to an already existent linux interrupt handler by
  rt_pend_linux_irq().
*/

static int timer_inuse=0;

#ifndef CONFIG_RTAI_TIMER_TRUE_PERIODIC
void rt_request_timer(void (*handler)(void), unsigned int tick, int unused) {
  volatile unsigned short *timerp;
  unsigned long flags;

  if(timer_inuse) return;
  timer_inuse=1;

  hard_save_flags_and_cli(flags);

  rt_times.tick_time = rdtsc();
  rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;

  if (tick > 0) {
    rt_times.linux_tick = LATCH;
    rt_times.intr_time = rt_times.tick_time + tick;
    rt_times.periodic_tick = tick;
  }
  else {
    rt_times.linux_tick = imuldiv(LATCH, tuned.cpu_freq, FREQ_8254);
    rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
    rt_times.periodic_tick = rt_times.linux_tick;
  };

  if(mach_tick) {
    lx_mach_tick = mach_tick;
    mach_tick = 0;
  };

  rt_free_global_irq(RT_TIMER_IRQ);
  rt_request_global_irq(RT_TIMER_IRQ,handler);
  rt_irq[RT_TIMER_IRQ].ack = rt_ack_tmr;
  rt_irq[RT_TIMER_IRQ].flags = 0;
#ifdef CONFIG_NETtel
  // we need to get an irq/mask relation to be able to pend timer irq:
  rt_request_global_irq(TIMER_8254_IRQ,0);
#endif

  timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE4);
  timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;

#ifdef CONFIG_NETtel
  do {
    volatile unsigned char  *icrp;
    icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_TIMER1ICR);
    *icrp = MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI3;
  } while(0);
#endif

#ifdef CONFIG_MOTOROLA
  do {
    volatile unsigned long  *icrp;
    icrp = (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1);
    *icrp = 0x0000000e; /* TMR4 with priority 6 */
  } while(0);
#endif
  
  timerp[MCFTIMER_TRR] = rt_times.intr_time;
  timerp[MCFTIMER_TMR] = MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK1 |
    MCFTIMER_TMR_ENABLE | ( ((MCF_CLK/CLOCK_TICK_RATE)-1) << 8 );	

  hard_restore_flags(flags);
  return;
}
#else
void rt_request_timer(void (*handler)(void), unsigned int tick, int unused) {
  volatile unsigned short *timerp;
  unsigned long flags;

  if(timer_inuse) return;
  timer_inuse=1;

  hard_save_flags_and_cli(flags);

  rt_times.tick_time = rdtsc();
  rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;

  if (tick > 0) {
    rt_times.linux_tick = LATCH;
    rt_times.intr_time = rt_times.tick_time + tick;
    rt_times.periodic_tick = tick;

    timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE4);
    timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;
    
#ifdef CONFIG_NETtel
    do {
      volatile unsigned char  *icrp;
      icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_TIMER1ICR);
      *icrp = MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI3;
    } while(0);
#endif

#ifdef CONFIG_MOTOROLA
    do {
      volatile unsigned long  *icrp;
      icrp = (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1);
      *icrp = 0x0000000e; /* TMR4 with priority 6 */
    } while(0);
#endif

    timerp[MCFTIMER_TRR] = tick;
    timerp[MCFTIMER_TMR] = MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK1 | MCFTIMER_TMR_RESTART |
      MCFTIMER_TMR_ENABLE | ( ((MCF_CLK/CLOCK_TICK_RATE)-1) << 8 );
  }
  else {
    rt_times.linux_tick = imuldiv(LATCH, tuned.cpu_freq, FREQ_8254);
    rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
    rt_times.periodic_tick = rt_times.linux_tick;

    timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE4);
    timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;
    
#ifdef CONFIG_NETtel
    icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_TIMER1ICR);
    *icrp = MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI3;
#endif

#ifdef CONFIG_MOTOROLA
    do {
      volatile unsigned long  *icrp;
      icrp = (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1);
      *icrp = 0x0000000e; /* TMR4 with priority 6 */
    } while(0);
#endif
    
    timerp[MCFTIMER_TRR] = rt_times.intr_time;
    timerp[MCFTIMER_TMR] = MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK1 |
    MCFTIMER_TMR_ENABLE | ( ((MCF_CLK/CLOCK_TICK_RATE)-1) << 8 );
  };

  if(mach_tick) {
    lx_mach_tick = mach_tick;
    mach_tick = 0;
  };

  rt_free_global_irq(RT_TIMER_IRQ);
  rt_request_global_irq(RT_TIMER_IRQ,handler);
  rt_irq[RT_TIMER_IRQ].ack = rt_ack_tmr;
  rt_irq[RT_TIMER_IRQ].flags = 0;
#ifdef CONFIG_NETtel
  // we need to get an irq/mask relation to be able to pend timer irq:
  rt_request_global_irq(TIMER_8254_IRQ,0);
#endif

  hard_restore_flags(flags);
  return;
}
#endif

void rt_free_timer(void) {
  volatile unsigned short *timerp;

  unsigned long flags;
  hard_save_flags_and_cli(flags);

  timer_inuse=0;  

  if(lx_mach_tick) {
    mach_tick = lx_mach_tick;
    lx_mach_tick = 0;
  };
  rt_free_global_irq(RT_TIMER_IRQ);
#ifdef CONFIG_NETtel
  rt_free_global_irq(TIMER_8254_IRQ);
#endif

  timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE4);
  timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;

#ifdef CONFIG_NETtel
  icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_TIMER1ICR);
  *icrp = MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL1 | MCFSIM_ICR_PRI3;
#endif

#ifdef CONFIG_MOTOROLA
  do {
    volatile unsigned long  *icrp;
    icrp = (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1);
    *icrp = 0x0000000c; /* TMR4 with priority 4 */
  } while(0);
#endif

  timerp[MCFTIMER_TRR] = rdtsc() + (CLOCK_TICK_RATE / HZ);
  timerp[MCFTIMER_TMR] = MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK1 |
    MCFTIMER_TMR_ENABLE | ( ((MCF_CLK/CLOCK_TICK_RATE)-1) << 8 );  

  hard_restore_flags(flags);
  return;
};



/* ------------------< misc stuff for external use, only >------------ */

// switching to rtai or linux mode
void rt_switch_to_linux(int cpuid) {global.used_by_linux=1;};
void rt_switch_to_real_time(int cpuid) {global.used_by_linux=0;};

// mount/umount
#define rtai_mounted 1
void rt_mount_rtai(void) {};
void rt_umount_rtai(void) {};

// task switching
void up_task_sw(void *current_task_ref, void *new_task) {
  __asm__ __volatile__ ("lea %%sp@(-60),%%sp\n\t"
			"movem.l %%d0-%%d7/%%a0-%%a6,%%sp@\n\t"
			"pea %%pc@(1f)\n\t"
			"move.l (%0),%%a1\n\t"
			"move.l %%sp,(%%a1)\n\t"
			"move.l %1,(%0)\n\t"
			"move.l %1,%%a1\n\t"
			"move.l (%%a1),%%sp\n\t"
			"rts\n\t"
			"1:  movem.l %%sp@,%%d0-%%d7/%%a0-%%a6\n\t"
			"lea %%sp@(60),%%sp\n\t"
			: /* no output */
			: "a" (current_task_ref) , "d" (new_task)
			: "%a1", "memory");
};

volatile unsigned int *locked_cpus = &global.locked_cpus;



/* ----------------< Module initialisation & cleanup >---------- */

static int CpuFreq = 0;
MODULE_PARM(CpuFreq, "i");

#ifdef CONFIG_PROC_FS
static int rtai_proc_register(void);
static void rtai_proc_unregister(void);
#endif

struct desc_struct trap_sys_vector;
struct desc_struct trap_srsave_vector;

int init_module(void) {
static void rt_printk_sysreq_handler(void);
  int flags;

  if (CpuFreq == 0) {
    CpuFreq = CLOCK_TICK_RATE;
  }
  tuned.cpu_freq = CpuFreq;
  printk("rtai: using clock_freq %d\n",tuned.cpu_freq);

  lx_irq_init();
  rt_irq_init();
  rt_srq_init();

  hard_save_flags_and_cli(flags);
  linux_rthal = rthal;
  rthal = rtai_rthal;
  sysrq[1].rtai_handler = rt_printk_sysreq_handler;
  trap_sys_vector = rt_set_full_intr_vect(RTAI_SYS_VECTOR,0,0,srqisr);
  trap_srsave_vector = rt_set_full_intr_vect(RTAI_SRSAVE_VECTOR,0,0,save_sr_trap);
  hard_restore_flags(flags);
  
#ifdef CONFIG_PROC_FS
  rtai_proc_register();
#endif

  return 0;
}

void cleanup_module(void) {

  int flags;

#ifdef CONFIG_PROC_FS
  rtai_proc_unregister();
#endif

  hard_save_flags_and_cli(flags);
  rthal = linux_rthal;
  rt_reset_full_intr_vect(RTAI_SYS_VECTOR,trap_sys_vector);
  rt_reset_full_intr_vect(RTAI_SRSAVE_VECTOR,trap_srsave_vector);
  hard_restore_flags(flags);
};


#if 1 // FIXME
/* ----------------------< proc filesystem section >----------------------*/

#ifdef CONFIG_PROC_FS

struct proc_dir_entry *rtai_proc_root = NULL;

static int rtai_read_rtai(char *page, char **start, off_t off, int count,
                          int *eof, void *data)
{

	PROC_PRINT_VARS;
        int i;

        PROC_PRINT("\nRTAI Real Time Kernel, Version: %s\n\n", RTAI_VERSION);
        PROC_PRINT("    RTAI mount count: %d\n", rtai_mounted);

        PROC_PRINT("\nRTAI IRQs\n");
	for (i = 0; i < NR_IRQS; i++) {
	  if (rt_irq[i].enabled) {
	    PROC_PRINT("%d ", i);
	  };  
	};

        PROC_PRINT("\nRTAI sysreqs in use: \n");
        for (i = 0; i < RT_NUM_SRQS; i++) {
          if (sysrq[i].rtai_handler || sysrq[i].user_handler) {
            PROC_PRINT("%d ", i);
          }
        }

        PROC_PRINT("\n\n");

	PROC_PRINT_DONE;
}       /* End function - rtai_read_rtai */

static int rtai_proc_register(void)
{

	struct proc_dir_entry *ent;

        rtai_proc_root = create_proc_entry("rtai", S_IFDIR, 0);
        if (!rtai_proc_root) {
		printk("Unable to initialize /proc/rtai\n");
                return(-1);
        }
	rtai_proc_root->owner = THIS_MODULE;
        ent = create_proc_entry("rtai", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
        if (!ent) {
		printk("Unable to initialize /proc/rtai/rtai\n");
                return(-1);
        }
	ent->read_proc = rtai_read_rtai;
        return(0);
}       /* End function - rtai_proc_register */


static void rtai_proc_unregister(void)
{
        remove_proc_entry("rtai", rtai_proc_root);
        remove_proc_entry("rtai", 0);
}       /* End function - rtai_proc_unregister */

#endif /* CONFIG_PROC_FS */


/* ------------------< rt_printk >------------------*/

// Our printk function, its use should be safe everywhere.
#include <linux/console.h>

int rtai_print_to_screen(const char *format, ...)
{
        static spinlock_t display_lock = SPIN_LOCK_UNLOCKED;
        static char display[25*80];
        unsigned long flags;
        struct console *c;
        va_list args;
        int len;

        flags = rt_spin_lock_irqsave(&display_lock);
        va_start(args, format);
        len = vsprintf(display, format, args);
        va_end(args);
        c = console_drivers;
        while(c) {
                if ((c->flags & CON_ENABLED) && c->write)
                        c->write(c, display, len);
                c = c->next;
	}
        rt_spin_unlock_irqrestore(flags, &display_lock);

	return len;
}

/*
 *  rt_printk.c, hacked from linux/kernel/printk.c.
 *
 * Modified for RT support, David Schleef.
 *
 * Adapted to RTAI, and restyled his way by Paolo Mantegazza. Now it has been
 * taken away from the fifos module and has become an integral part of the basic
 * RTAI module.
 */

#define PRINTK_BUF_LEN	(4096*2) // Some test programs generate much output. PC
#define TEMP_BUF_LEN	(256)

static char rt_printk_buf[PRINTK_BUF_LEN];
static int buf_front, buf_back;

static char buf[TEMP_BUF_LEN];

int rt_printk(const char *fmt, ...)
{
        static spinlock_t display_lock = SPIN_LOCK_UNLOCKED;
	va_list args;
	int len, i;
	unsigned long flags;

        flags = rt_spin_lock_irqsave(&display_lock);
	va_start(args, fmt);
	len = vsprintf(buf, fmt, args);
	va_end(args);
	if (buf_front + len >= PRINTK_BUF_LEN) {
		i = PRINTK_BUF_LEN - buf_front;
		memcpy(rt_printk_buf + buf_front, buf, i);
		memcpy(rt_printk_buf, buf + i, len - i);
		buf_front = len - i;
	} else {
		memcpy(rt_printk_buf + buf_front, buf, len);
		buf_front += len;
	}
        rt_spin_unlock_irqrestore(flags, &display_lock);
	rt_pend_linux_srq(1);

	return len;
}

static void rt_printk_sysreq_handler(void)
{
	int tmp;

	while(1) {
		tmp = buf_front;
		if (buf_back  > tmp) {
			printk("%.*s", PRINTK_BUF_LEN - buf_back, rt_printk_buf + buf_back);
			buf_back = 0;
		}
		if (buf_back == tmp) {
			break;
		}
		printk("%.*s", tmp - buf_back, rt_printk_buf + buf_back);
		buf_back = tmp;
	}
}

#endif
