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
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	(void)old;

	*ret = newas;
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

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize, off_t elf_offset, size_t elfsize,
		 int readable, int writeable, int executable)
{
	size_t npages;
	vaddr_t first_vaddr = vaddr;
	vaddr_t last_vaddr = vaddr + memsize;

	KASSERT(as != NULL);

	//vm_can_sleep();

	/* Align the region. First, the base... */
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
	

	/* ...and now the length. */
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;
	
	npages = memsize / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->s_text == NULL) {
		as->s_text = segment_create();
		segment_define(as->s_text, elf_offset, vaddr, first_vaddr, last_vaddr, npages, elfsize);
		return 0;
	}

	if (as->s_data == NULL) {
		as->s_data = segment_create();
		segment_define(as->s_data, elf_offset, vaddr, first_vaddr, last_vaddr, npages, elfsize);
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("vm: Warning: too many regions\n");
	return ENOSYS;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as != NULL);

	as->s_stack = segment_create();
	segment_define(as->s_stack, 0, USERSTACK - VM_STACKPAGES * PAGE_SIZE, USERSTACK - VM_STACKPAGES * PAGE_SIZE, MIPS_KSEG0, VM_STACKPAGES, 0);
	
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
    
    if (vaddr >= as->s_text->first_vaddr && vaddr < as->s_text->last_vaddr)
    {
        return as->s_text;
    }

    if (vaddr >= as->s_data->first_vaddr && vaddr < as->s_data->last_vaddr)
    {
        return as->s_data;
    }

    if (vaddr >= as->s_stack->first_vaddr && vaddr < as->s_stack->last_vaddr)
    {
        return as->s_stack;
    }
    
    return NULL;
}

off_t
as_get_elf_offset(struct addrspace *as, vaddr_t vaddr)
{
	struct segment *seg;

	KASSERT(as != NULL);

	seg = as_get_segment(as, vaddr);
	if(seg == NULL)
	{
		panic("Cannot retrieve as segment");
	}

	return seg->elf_offset - seg->base_vaddr + vaddr;
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

	if(vaddr - seg -> base_vaddr <= ((seg->elfsize + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE ) return true;
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
    
    panic("Could not find the segment");
}

int as_load_page(struct addrspace *as,struct vnode *vnode, vaddr_t faultaddress){
	struct segment *segment;
	struct pt_entry *pt_row;
	off_t offset;				/* 	offset within the elf				*/
	size_t size;				/* 	size of memory to load from elf 	*/
	paddr_t target_addr;

	pt_row = pt_get_entry(as,faultaddress );
	segment = as_get_segment(as,faultaddress);

	if(segment->base_vaddr == ( faultaddress & PAGE_FRAME )){
		/*	first page of the segment	*/

		size = PAGE_SIZE -( segment->first_vaddr & ~PAGE_FRAME );
		offset = segment->elf_offset;
		target_addr = pt_row->frame_index * PAGE_SIZE + ( segment->first_vaddr & ~PAGE_FRAME ) ;

	}else if((segment->last_vaddr & PAGE_FRAME) == (( faultaddress & PAGE_FRAME ))){
		/* 	last page of the segment	*/

		size = ( segment->last_vaddr & ~PAGE_FRAME );
		offset = segment->elf_offset + 									/*	offset within the elf	*/
				PAGE_SIZE -( segment->first_vaddr & ~PAGE_FRAME ) + 	/* 	size of the first page	*/
				( segment->npages - 2 ) * PAGE_SIZE ;					/*	pages in the middle		*/
		target_addr = pt_row->frame_index * PAGE_SIZE;

	}else{
		/*	middle page of the segment	*/

		size = PAGE_SIZE;
		offset = segment->elf_offset + 									/*	offset within the elf	*/
				( faultaddress & PAGE_FRAME ) - segment->base_vaddr - 
				PAGE_SIZE -( segment->first_vaddr & ~PAGE_FRAME );
		target_addr = pt_row->frame_index * PAGE_SIZE;

	}

	
	load_page(vnode,offset,target_addr,size);
	return 0;
}

#endif /* OPT_RUDEVM */
