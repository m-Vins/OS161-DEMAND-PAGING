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

	as->s_data = NULL;
	as->s_text = NULL;
	as->s_stack = NULL;
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

void
as_destroy(struct addrspace *as)
{
	KASSERT(as != NULL);
	
	int pt_size = as->s_data->npages + as->s_text->npages + as->s_stack->npages;

	pt_empty(as->as_ptable, pt_size);
	pt_destroy(as->as_ptable);
	segment_destroy(as->s_text);
	segment_destroy(as->s_data);
	segment_destroy(as->s_stack);

	kfree(as);
}

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
 * @brief Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
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

	memsize += first_vaddr & ~(vaddr_t)PAGE_FRAME;
	npages =  DIVROUNDUP(memsize,PAGE_SIZE);

	base_vaddr = first_vaddr & PAGE_FRAME;
	
	if (as->s_text == NULL) {
		as->s_text = segment_create();
		segment_define(as->s_text, elf_offset, base_vaddr, first_vaddr, last_vaddr, npages, elfsize);
		return 0;
	}

	if (as->s_data == NULL) {
		as->s_data = segment_create();
		segment_define(as->s_data, elf_offset, base_vaddr, first_vaddr, last_vaddr, npages, elfsize);
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	panic("vm: Warning: too many regions");
	return ENOSYS;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as != NULL);

	as->s_stack = segment_create();
	segment_define(as->s_stack, 0, USERSTACK - VM_STACKPAGES * PAGE_SIZE, USERSTACK - VM_STACKPAGES * PAGE_SIZE, USERSTACK, VM_STACKPAGES, 0);
	
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

int
as_define_pt(struct addrspace *as)
{
	KASSERT(as != NULL);

	/* Create the page table based on the segments loaded previously */
	as->as_ptable = pt_create(as->s_data->npages + as->s_text->npages + as->s_stack->npages);

	return 0;
}

static
struct segment *
as_get_segment(struct addrspace *as, vaddr_t vaddr){

	KASSERT(as != NULL);

	switch(as_get_segment_type(as,vaddr)){
		case SEGMENT_TEXT:
			return as->s_text;
		case SEGMENT_DATA:	
			return as->s_data;
		case SEGMENT_STACK:	
			return as->s_stack;
		default:
			panic("vaddr out of range! (as_get_segment)");
	}
    return NULL;
}


bool as_check_in_elf(struct addrspace *as, vaddr_t vaddr){
	struct segment *seg;

	KASSERT(as != NULL);

	seg = as_get_segment(as, vaddr);
	if(seg == NULL)
	{
		panic("Cannot retrieve as segment");
	}

	KASSERT(vaddr >= seg->first_vaddr);
	KASSERT(vaddr < seg->last_vaddr);

	if(vaddr < ROUNDUP(seg->first_vaddr + seg->elfsize,PAGE_SIZE)) return true;
	return false;
}

int
as_get_segment_type(struct addrspace *as, vaddr_t vaddr)
{
	KASSERT(as != NULL);
    
	if (vaddr >= as->s_text->first_vaddr && vaddr < as->s_text->last_vaddr)
    {
        return SEGMENT_TEXT;
    }

    if (vaddr >= as->s_data->first_vaddr && vaddr < as->s_data->last_vaddr)
    {
        return SEGMENT_DATA;
    }

    if (vaddr >= as->s_stack->first_vaddr && vaddr < as->s_stack->last_vaddr)
    {
        return SEGMENT_STACK;
    }
    
    panic("vaddr out of range! (as_get_segment_type)");
}

int as_load_page(struct addrspace *as,struct vnode *vnode, vaddr_t faultaddress){
	struct segment *segment;
	struct pt_entry *pt_row;
	off_t offset;				/* 	offset within the elf				*/
	size_t size;				/* 	size of memory to load from elf 	*/
	paddr_t target_addr;

	pt_row = pt_get_entry(as,faultaddress);
	segment = as_get_segment(as,faultaddress);

	/*assert that the fault address belongs to the segment 	*/
	KASSERT(faultaddress < ROUNDUP(segment->first_vaddr + segment->elfsize,PAGE_SIZE));
	KASSERT(faultaddress >= segment->base_vaddr);

	if(segment->base_vaddr == ( faultaddress & PAGE_FRAME )){
		/*	first page of the segment	*/

		/**
		 * The portion belonging to the first page of the segment
		 * which has to be loaded from the elf has a size equal to 
		 * the number of bytes starting from the first virtual address
		 * up to the first virtual address of the next page.
		 * It can be that the size of the segment within the elf
		 * is even smaller, in this case, we only have to load this
		 * portion into the right address.
		 * 
		 */
		size = PAGE_SIZE - ( segment->first_vaddr & ~PAGE_FRAME ) > segment->elfsize ? 
				segment->elfsize :								/* in case the elfsize is smaller		*/
				(PAGE_SIZE -( segment->first_vaddr & ~PAGE_FRAME )) ;	
		offset = segment->elf_offset ;							/*  offset within the elf				*/
		target_addr = pt_row->frame_index * PAGE_SIZE 			/*  physycal base address				*/
					+ ( segment->first_vaddr & ~PAGE_FRAME ) ;	/* 	offset within the segment 			*/

	}else if(((segment->first_vaddr + segment->elfsize) & PAGE_FRAME) == (( faultaddress & PAGE_FRAME ))){
		/* 	last page of the segment (concerning the pages within the elf)	*/

		/**
		 * The size to be loaded is equal to the size of the last page 
		 * (among the ones within the subset of pages to be loaded from 
		 * the elf file) within the elf
		 * 
		 */
		size = (segment->first_vaddr + segment->elfsize) & ~PAGE_FRAME ;
		offset = segment->elf_offset + 							/*	offset within the elf				*/
				(faultaddress & PAGE_FRAME) -					/*  base address of the faulting page	*/
				segment->first_vaddr ;							/*  first vaddr of the segment			*/
		target_addr = pt_row->frame_index * PAGE_SIZE;			/*	physical addr of the faulting page	*/

	}else{
		/*	middle page of the segment	*/

		size = PAGE_SIZE;										
		offset = segment->elf_offset + 							/*	offset within the elf				*/
				(faultaddress & PAGE_FRAME) -					/*  base address of the faulting page	*/
				segment->first_vaddr ;							/*  first vaddr of the segment			*/
		target_addr = pt_row->frame_index * PAGE_SIZE;			/*	physical addr of the faulting page	*/

	}

	
	load_page(vnode,offset,target_addr,size);

	return 0;
}

#endif /* OPT_RUDEVM */
