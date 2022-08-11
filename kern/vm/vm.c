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
#include <vm.h>
#include <coremap.h>
#include <addrspace.h>
#include <vm_tlb.h>
#include "opt-rudevm.h"


#if OPT_RUDEVM
/* under vm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */


static struct spinlock vm_lock = SPINLOCK_INITIALIZER;

void
vm_bootstrap(void)
{
	/* Do nothing. */
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
 * @param kernel KERNEL if the page is requested from the kernel, USER otherwise.
 * @return paddr_t the first physical address of the requested pages.
 */
static
paddr_t
getppages(unsigned long npages, char kernel)
{
	paddr_t addr;

	spinlock_acquire(&vm_lock);
	addr = coremap_getppages(npages,kernel);
	spinlock_release(&vm_lock);
	
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
	spinlock_acquire(&vm_lock);
	coremap_freeppages(addr);
	spinlock_release(&vm_lock);
} 

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	vm_can_sleep();
	pa = getppages(npages, COREMAP_KERNEL);
	if (pa==0) {
		return 0;
	}
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
static
paddr_t 
alloc_upage(){
	paddr_t pa;

	/* the user can alloc one page at a time */
	pa = getppages(1, COREMAP_USER);
	
	return pa;
}

/**
 * @brief deallocate the given page for the user.
 * 
 * @param addr 
 */
// static
// void free_upage(paddr_t addr){
// 	freeppages(addr);
// };

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
	off_t elf_offset;

	/* Obtain the first address of the page */
	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "vm: fault: 0x%x\n", faultaddress);

	switch (faulttype)
	{
	    case VM_FAULT_READONLY:
			panic("vm: got VM_FAULT_READONLY\n");
			// TODO: exit from process;
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
	KASSERT(as->s_data != NULL);
	KASSERT(as->s_stack != NULL);
	KASSERT(as->s_text != NULL);
	KASSERT(as->as_ptable != NULL);

	pt_row = pt_get_entry(faultaddress, as);
	switch(pt_row->status)
	{
		case NOT_LOADED:
			elf_offset = as_get_elf_offset(faultaddress, as);
			page_paddr = alloc_upage();
			load_page(curproc->p_vnode, elf_offset, page_paddr);
			pt_row->status = IN_MEMORY;
			pt_row->frame_index = page_paddr / PAGE_SIZE;
		case IN_MEMORY:
			break;
		default:
			panic("Cannot resolve fault");
	}

	tlb_insert(faultaddress, pt_row->frame_index * PAGE_SIZE, false); // TODO: check permissions

	return 0;
}
#endif /* OPT_RUDEVM */

