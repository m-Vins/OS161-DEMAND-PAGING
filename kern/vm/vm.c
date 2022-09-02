#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <pt.h>
#include <vm.h>
#include <coremap.h>
#include <vm_tlb.h>
#include "opt-rudevm.h"
#include "syscall.h"
#include <swapfile.h>
#include "opt-stats.h"

#if OPT_STATS
#include <vmstats.h>
#endif

#if OPT_RUDEVM
/* under vm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */

void
vm_bootstrap(void)
{
#if OPT_SWAP
	swap_bootstrap();
#endif
}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in vm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * vm starts blowing up during the VM assignment.
 */
static
void
vm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

/**
 * @brief get npages from the coremap.
 * 
 * @param npages 
 * @param ptentry pointer to the pt entry, NULL if kernel's page.
 * @return paddr_t the first physical address of the requested pages.
 */
static
paddr_t
getppages(unsigned long npages, struct pt_entry *ptentry)
{
	paddr_t addr;

	addr = coremap_getppages(npages, ptentry);
	if (addr == 0) {
		panic("Out of memory");
	}

	return addr;
}

/**
 * @brief free the allocated pages starting from addr.
 * 
 * @param addr 
 */
static 
void
freeppages(paddr_t addr){
	coremap_freeppages(addr);
} 

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	vm_can_sleep();
	pa = getppages(npages, NULL);
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
	/* get the physical address */
	paddr_t pa = KVADDR_TO_PADDR(addr);
	freeppages(pa);
}

/**
 * @brief allocate a page for the user. 
 * It is different from the alloc_kpage as it allocate one frame at a time .
 * 
 * @return paddr_t the virtual address of the allocated frame
 */
paddr_t
alloc_upage(struct pt_entry *pt_row){
	paddr_t pa;

	vm_can_sleep();
	/* the user can alloc one page at a time */
	pa = getppages(1, pt_row);
	return pa;
}

/**
 * @brief deallocate the given page for the user.
 * 
 * @param addr 
 */

void free_upage(paddr_t addr){
	freeppages(addr);
};

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	struct pt_entry *pt_row;
	struct addrspace *as;
	paddr_t page_paddr;
	int seg_type;
	int readonly;
	vaddr_t basefaultaddr;

#if OPT_STATS
	vmstats_hit(VMSTAT_TLB_FAULT);
#endif

	/* Obtain the first address of the page */
	basefaultaddr = faultaddress & PAGE_FRAME;

	DEBUG(DB_VM, "vm: fault: 0x%x\n", faultaddress);

	switch (faulttype)
	{
 	    case VM_FAULT_READONLY:
			kprintf("vm: got VM_FAULT_READONLY, process killed\n");
			sys__exit(-1);
			return 0;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
			break;
	    default:
			return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_data != NULL);
	KASSERT(as->as_stack != NULL);
	KASSERT(as->as_text != NULL);
	KASSERT(as->as_ptable != NULL);

	pt_row = pt_get_entry(as, faultaddress);
	seg_type = as_get_segment_type(as, faultaddress);
	switch(pt_row->pt_status)
	{
		case NOT_LOADED:
			/*	alloc a page				*/
			page_paddr = alloc_upage(pt_row);

			/**  
			 * update page table.			
			 * it is important to do it before as_load_page
			 * as the pt_entry will be used to retrieve the
			 * physical address of the page
			 */
			pt_set_entry(pt_row,page_paddr,0,IN_MEMORY);

			/*	load the page if needed 	*/
			if(seg_type != SEGMENT_STACK && as_check_in_elf(as,faultaddress))
			{
				as_load_page(as,curproc->p_vnode,faultaddress);
			}
#if OPT_STATS
			else
			{
    			vmstats_hit(VMSTAT_PAGE_FAULT_ZERO);
			}
#endif
			break;
		case IN_MEMORY:
#if OPT_STATS
    		vmstats_hit(VMSTAT_TLB_RELOAD);
#endif
			break;
		case IN_SWAP:
#if OPT_SWAP
			/*	alloc the page				*/
			page_paddr = alloc_upage(pt_row);

			/*	swap it from the elf file into memory	*/
			swap_in(page_paddr, pt_row->pt_swap_index);

			/* update page table	*/
			pt_set_entry(pt_row,page_paddr,0,IN_MEMORY);

#else
			panic("swap not implemented!");
#endif
		break;
		default:
			panic("Cannot resolve fault");
	}

	KASSERT(seg_type != 0);
	readonly = seg_type == SEGMENT_TEXT;

	/* update tlb	*/
	tlb_insert(basefaultaddr, pt_row->pt_frame_index * PAGE_SIZE, readonly); 

	return 0;
}
#endif /* OPT_RUDEVM */
