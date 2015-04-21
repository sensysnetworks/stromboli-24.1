/*
    bttv - Bt848 frame grabber driver

    Copyright (C) 1996,97,98 Ralph  Metzler (rjkm@thp.uni-koeln.de)
                           & Marcus Metzler (mocm@thp.uni-koeln.de)
    (c) 1999,2000 Gerd Knorr <kraxel@goldbach.in-berlin.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    REMARK. The original idea of copying it from this kernel file is due to 
    T. Motylewsky. Since it is now simpler and clearer than it was before there 
    is no more any need to follow Tomek way of doing it. (Paolo Mantegazza)
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <linux/ioport.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <linux/types.h>
#include <linux/wrapper.h>
#include <linux/interrupt.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>

/* [DaveM] I've recoded most of this so that:
 * 1) It's easier to tell what is happening
 * 2) It's more portable, especially for translating things
 *    out of vmalloc mapped areas in the kernel.
 * 3) Less unnecessary translations happen.
 *
 * The code used to assume that the kernel vmalloc mappings
 * existed in the page tables of every process, this is simply
 * not guarenteed.  We now use pgd_offset_k which is the
 * defined way to get at the kernel page tables.
 */

/* Given PGD from the address space's page table, return the kernel
 * virtual mapping of the physical memory mapped at ADR.
 */
static inline unsigned long uvirt_to_kva(pgd_t *pgd, unsigned long adr)
{
        unsigned long ret = 0UL;
	pmd_t *pmd;
	pte_t *ptep, pte;
  
	if (!pgd_none(*pgd)) {
                pmd = pmd_offset(pgd, adr);
                if (!pmd_none(*pmd)) {
                        ptep = pte_offset(pmd, adr);
                        pte = *ptep;
                        if(pte_present(pte))
                                ret = (page_address(pte_page(pte))|(adr&(PAGE_SIZE-1)));
                }
        }
	return ret;
}

static inline unsigned long uvirt_to_bus(unsigned long adr) 
{
        unsigned long kva, ret;

        kva = uvirt_to_kva(pgd_offset(current->mm, adr), adr);
	ret = virt_to_bus((void *)kva);
        return ret;
}

static inline unsigned long kvirt_to_bus(unsigned long adr) 
{
        unsigned long va, kva, ret;

        va = VMALLOC_VMADDR(adr);
        kva = uvirt_to_kva(pgd_offset_k(va), va);
	ret = virt_to_bus((void *)kva);
        return ret;
}

/* Here we want the physical address of the memory.
 * This is used when initializing the contents of the
 * area and marking the pages as reserved.
 */
static inline unsigned long kvirt_to_pa(unsigned long adr) 
{
        unsigned long va, kva, ret;

        va = VMALLOC_VMADDR(adr);
        kva = uvirt_to_kva(pgd_offset_k(va), va);
	ret = __pa(kva);
        return ret;
}

static void * rvmalloc(signed long size)
{
	void * mem;
	unsigned long adr, page;

	mem=vmalloc(size);
	if (mem) 
	{
// In rtai_shm we prefer to do it elswhere.
//		memset(mem, 0, size); /* Clear the ram out, no junk to the user */
	        adr=(unsigned long) mem;
		while (size > 0) 
                {
	                page = kvirt_to_pa(adr);
			mem_map_reserve(MAP_NR(__va(page)));
			adr+=PAGE_SIZE;
			size-=PAGE_SIZE;
		}
	}
	return mem;
}

static void rvfree(void * mem, signed long size)
{
        unsigned long adr, page;
        
	if (mem) 
	{
	        adr=(unsigned long) mem;
		while (size > 0) 
                {
	                page = kvirt_to_pa(adr);
			mem_map_unreserve(MAP_NR(__va(page)));
			adr+=PAGE_SIZE;
			size-=PAGE_SIZE;
		}
		vfree(mem);
	}
}

/* 
 * REMARK: the following is an adaption of do_btts_mmap to the need of rtai_shm.
 */

static int rvmmap(void *vadr, unsigned size, struct vm_area_struct *vma) 
{
	unsigned long start, offset, page, pos;

	start = vma->vm_start;
	pos = (unsigned long)vadr + (offset = vma->vm_pgoff << PAGE_SHIFT);
	if (offset < 0 || (vma->vm_end - vma->vm_start) > size ||
	    pos%PAGE_SIZE || start%PAGE_SIZE || size%PAGE_SIZE) {
		return -EFAULT;
	}
	while (size > 0) {
		page = kvirt_to_pa(pos);
		if (remap_page_range(start, page, PAGE_SIZE, vma->vm_page_prot)) {
                        return -EAGAIN;
		}
		start+=PAGE_SIZE;
		pos+=PAGE_SIZE;
		size-=PAGE_SIZE;
	}
	return 0;
}
