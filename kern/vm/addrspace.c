/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <segment.h>
#include <vm_tlb.h>
#include <pt.h>
#include "opt-stats.h"
#if OPT_STATS
#include <vmstats.h>
#endif


#define VM_STACKPAGES    18

#if OPT_RUDEVM
struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	as->as_data = NULL;
	as->as_text = NULL;
	as->as_stack = NULL;
	as->as_ptable = NULL;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	(void)old;
	(void)ret;
	panic("as_copy() still not implemented!");

	return 0;
}

/**
 * @brief 	deallocates both the pages in memory and in the swapfile, 
 * 		   	then frees all the data structures of the address space.
 * 
 * @param as 
 */
void
as_destroy(struct addrspace *as)
{
	KASSERT(as != NULL);
	
	int pt_size = as->as_data->seg_npages + as->as_text->seg_npages + as->as_stack->seg_npages;

	pt_empty(as->as_ptable, pt_size);
	pt_destroy(as->as_ptable);
	segment_destroy(as->as_text);
	segment_destroy(as->as_data);
	segment_destroy(as->as_stack);

	kfree(as);
}

/**
 * @brief 	if the process is a USER process, the tlb
 * 			is totally invalidated.
 * 
 */
void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	tlb_invalidate();
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/**
 * @brief Set up a segment at virtual address FIRST_VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * BASE_VADDR + NPAGES*PAGE_SIZE .
 * 
 * @param as address space of the process
 * @param first_vaddr actual first virtual address of the segment
 * @param memsize size of the segment expressed in bytes
 * @param elf_offset offset of the segment within the elf file
 * @param elfsize size of the segment within the elf file
 * @return int 
 */
int
as_define_region(struct addrspace *as, vaddr_t first_vaddr, size_t memsize, off_t elf_offset, size_t elfsize)
{
	size_t npages;
	vaddr_t last_vaddr = first_vaddr + memsize;
	vaddr_t base_vaddr;

	KASSERT(as != NULL);
	KASSERT(memsize != 0);

	/*		compute the number of pages needed for the segment		*/
	memsize += first_vaddr & ~(vaddr_t)PAGE_FRAME;
	npages =  DIVROUNDUP(memsize,PAGE_SIZE);

	/*		compute the address of the first page of the segment	*/
	base_vaddr = first_vaddr & PAGE_FRAME;
	
	if (as->as_text == NULL) {
		as->as_text = segment_create();
		segment_define(as->as_text, elf_offset, base_vaddr, first_vaddr, last_vaddr, npages, elfsize);
		return 0;
	}

	if (as->as_data == NULL) {
		as->as_data = segment_create();
		segment_define(as->as_data, elf_offset, base_vaddr, first_vaddr, last_vaddr, npages, elfsize);
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	panic("vm: Warning: too many regions");
	return ENOSYS;
}

/**
 * @brief set up a segment for the stack.
 * 
 * @param as 
 * @param stackptr 
 * @return int 
 */
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as != NULL);

	as->as_stack = segment_create();
	segment_define(as->as_stack, 0, USERSTACK - VM_STACKPAGES * PAGE_SIZE, USERSTACK - VM_STACKPAGES * PAGE_SIZE, USERSTACK, VM_STACKPAGES, 0);
	
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

/**
 * @brief setup the page table for the address space
 * 
 * @param as 
 * @return int 
 */
int
as_define_pt(struct addrspace *as)
{
	KASSERT(as != NULL);

	/* Create the page table based on the segments loaded previously */
	int npages = as->as_data->seg_npages + as->as_text->seg_npages + as->as_stack->seg_npages;
	as->as_ptable = pt_create(npages);

	return 0;
}

/**
 * @brief retrieve the segment type from which the virtual address belongs to.
 * 
 * @param as 
 * @param vaddr 
 * @return int 
 */
int
as_get_segment_type(struct addrspace *as, vaddr_t vaddr)
{
	KASSERT(as != NULL);
    
	if (vaddr >= as->as_text->seg_first_vaddr && vaddr < as->as_text->seg_last_vaddr)
    {
        return SEGMENT_TEXT;
    }

    if (vaddr >= as->as_data->seg_first_vaddr && vaddr < as->as_data->seg_last_vaddr)
    {
        return SEGMENT_DATA;
    }

    if (vaddr >= as->as_stack->seg_first_vaddr && vaddr < as->as_stack->seg_last_vaddr)
    {
        return SEGMENT_STACK;
    }
    
    return 0;
}

/**
 * @brief retrieve the segment from which the virtual address belongs to.
 * 
 * @param as 
 * @param vaddr 
 * @return struct segment* 
 */
static
struct segment *
as_get_segment(struct addrspace *as, vaddr_t vaddr){

	KASSERT(as != NULL);

	switch(as_get_segment_type(as,vaddr)){
		case SEGMENT_TEXT:
			return as->as_text;
		case SEGMENT_DATA:	
			return as->as_data;
		case SEGMENT_STACK:	
			return as->as_stack;
		default:
			panic("invalid segment type! (as_get_segment)");
	}
    return NULL;
}

/**
 * @brief check whether the virtual address belongs to a page of
 * the segment which has to be load from the elf file.
 * 
 * @param as 
 * @param vaddr 
 * @return true if the page has to be loaded from the elf
 * @return false if not
 */
bool as_check_in_elf(struct addrspace *as, vaddr_t vaddr){
	struct segment *seg;

	KASSERT(as != NULL);

	seg = as_get_segment(as, vaddr);
	if(seg == NULL)
	{
		panic("Cannot retrieve as segment");
	}

	KASSERT(vaddr >= seg->seg_first_vaddr);
	KASSERT(vaddr < seg->seg_last_vaddr);

	if(vaddr < ROUNDUP(seg->seg_first_vaddr + seg->seg_elf_size,PAGE_SIZE)) return true;
	return false;
}

/**
 * @brief 	load a page from the elf file to the physical frame assigned 
 * to it. Before the actual loading of the page, we need to compute :
 * - the size 
 * - the offset within the elf 
 * - the target address where to store it
 * In order to compute those information, it is needed to differentiate
 * three cases:
 * - the faultaddress belongs to the first page of the segment
 * - the faultaddress belongs to the last page of the segment
 * - the faultaddress belongs to a middle page of the segment
 * 
 * @param as 
 * @param vnode 
 * @param faultaddress 
 * @return int 
 */
int as_load_page(struct addrspace *as,struct vnode *vnode, vaddr_t faultaddress){
	struct segment *segment;
	struct pt_entry *pt_row;
	off_t offset;				/* 	offset within the elf				*/
	size_t size;				/* 	size of memory to load from elf 	*/
	paddr_t target_addr;

#if OPT_STATS
	vmstats_hit(VMSTAT_PAGE_FAULT_DISK);
	vmstats_hit(VMSTAT_PAGE_FAULT_ELF);
#endif

	pt_row = pt_get_entry(as,faultaddress);
	segment = as_get_segment(as,faultaddress);

	/*	assert that the fault address belongs to the segment 	*/
	KASSERT(faultaddress < ROUNDUP(segment->seg_first_vaddr + segment->seg_elf_size,PAGE_SIZE));
	KASSERT(faultaddress >= segment->seg_base_vaddr);

	if(segment->seg_base_vaddr == ( faultaddress & PAGE_FRAME )){
		/*	first page of the segment	*/

		/**
		 * The portion belonging to the first page of the segment
		 * which has to be loaded from the elf has the size equal to 
		 * the number of bytes starting from the first virtual address
		 * up to the first virtual address of the next page.
		 * It can be that the size of the segment within the elf
		 * is even smaller, in this case, we only have to load this
		 * portion into the right address.
		 * 
		 */
		size = PAGE_SIZE - ( segment->seg_first_vaddr & ~PAGE_FRAME ) > segment->seg_elf_size ? 
				segment->seg_elf_size :								/* in case the elfsize is smaller		*/
				(PAGE_SIZE - ( segment->seg_first_vaddr & ~PAGE_FRAME )) ;	
		offset = segment->seg_elf_offset ;							/*  offset within the elf				*/
		target_addr = pt_row->pt_frame_index * PAGE_SIZE 			/*  physycal base address				*/
					+ ( segment->seg_first_vaddr & ~PAGE_FRAME ) ;	/* 	offset within the segment 			*/

	}else if(((segment->seg_first_vaddr + segment->seg_elf_size) & PAGE_FRAME) == ( faultaddress & PAGE_FRAME )){
		/* 	last page of the segment (concerning the pages within the elf)	*/


		/**
		 * The size to be loaded is equal to the size of the last page  
		 * (among the ones within the subset of pages to be loaded from 
		 * the elf file) within the elf
		 * 
		 */
		size = (segment->seg_first_vaddr + segment->seg_elf_size) & ~PAGE_FRAME ;
		offset = segment->seg_elf_offset + 							/*	offset within the elf				*/
				(faultaddress & PAGE_FRAME) -					/*  base address of the faulting page	*/
				segment->seg_first_vaddr ;							/*  first vaddr of the segment			*/
		target_addr = pt_row->pt_frame_index * PAGE_SIZE;			/*	physical addr of the faulting page	*/

	}else{
		/*	middle page of the segment	*/

		size = PAGE_SIZE;										
		offset = segment->seg_elf_offset + 							/*	offset within the elf				*/
				(faultaddress & PAGE_FRAME) -					/*  base address of the faulting page	*/
				segment->seg_first_vaddr ;							/*  first vaddr of the segment			*/
		target_addr = pt_row->pt_frame_index * PAGE_SIZE;			/*	physical addr of the faulting page	*/

	}

	load_page(vnode,offset,target_addr,size);

	return 0;
}

#endif /* OPT_RUDEVM */
