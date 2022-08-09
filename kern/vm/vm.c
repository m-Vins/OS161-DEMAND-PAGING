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


/* under vm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define VM_STACKPAGES    18

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
	paddr_t pa = addr - MIPS_KSEG0;
	spinlock_acquire(&vm_lock);
	freeppages(pa);
	spinlock_release(&vm_lock);
}

/**
 * @brief allocate a page for the user. 
 * It is different from the alloc_kpage as it allocate one frame at a time .
 * 
 * @return paddr_t the virtual address of the allocated frame
 */
// static
// paddr_t 
// alloc_upage(){
// 	paddr_t pa;

// 	/* the user can alloc one page at a time */
// 	pa = getppages(1, COREMAP_COREMAP_USER);
	
// 	return pa;
// }

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
    (void)faulttype;
    (void)faultaddress;
    return 0;
	// vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	// paddr_t paddr;
	// int i;
	// uint32_t ehi, elo;
	// struct addrspace *as;
	// int spl;

	// faultaddress &= PAGE_FRAME;

	// DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	// switch (faulttype) {
	//     case VM_FAULT_READONLY:
	// 	/* We always create pages read-write, so we can't get this */
	// 	panic("dumbvm: got VM_FAULT_READONLY\n");
	//     case VM_FAULT_READ:
	//     case VM_FAULT_WRITE:
	// 	break;
	//     default:
	// 	return EINVAL;
	// }

	// if (curproc == NULL) {
	// 	/*
	// 	 * No process. This is probably a kernel fault early
	// 	 * in boot. Return EFAULT so as to panic instead of
	// 	 * getting into an infinite faulting loop.
	// 	 */
	// 	return EFAULT;
	// }

	// as = proc_getas();
	// if (as == NULL) {
	// 	/*
	// 	 * No address space set up. This is probably also a
	// 	 * kernel fault early in boot.
	// 	 */
	// 	return EFAULT;
	// }

	// /* Assert that the address space has been set up properly. */
	// KASSERT(as->as_vbase1 != 0);
	// KASSERT(as->as_pbase1 != 0);
	// KASSERT(as->as_npages1 != 0);
	// KASSERT(as->as_vbase2 != 0);
	// KASSERT(as->as_pbase2 != 0);
	// KASSERT(as->as_npages2 != 0);
	// KASSERT(as->as_stackpbase != 0);
	// KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	// KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	// KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	// KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	// KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	// vbase1 = as->as_vbase1;
	// vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	// vbase2 = as->as_vbase2;
	// vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	// stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	// stacktop = USERSTACK;

	// if (faultaddress >= vbase1 && faultaddress < vtop1) {
	// 	paddr = (faultaddress - vbase1) + as->as_pbase1;
	// }
	// else if (faultaddress >= vbase2 && faultaddress < vtop2) {
	// 	paddr = (faultaddress - vbase2) + as->as_pbase2;
	// }
	// else if (faultaddress >= stackbase && faultaddress < stacktop) {
	// 	paddr = (faultaddress - stackbase) + as->as_stackpbase;
	// }
	// else {
	// 	return EFAULT;
	// }

	// /* make sure it's page-aligned */
	// KASSERT((paddr & PAGE_FRAME) == paddr);

	// /* Disable interrupts on this CPU while frobbing the TLB. */
	// spl = splhigh();

	// for (i=0; i<NUM_TLB; i++) {
	// 	tlb_read(&ehi, &elo, i);
	// 	if (elo & TLBLO_VALID) {
	// 		continue;
	// 	}
	// 	ehi = faultaddress;
	// 	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	// 	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	// 	tlb_write(ehi, elo, i);
	// 	splx(spl);
	// 	return 0;
	// }

	// kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	// splx(spl);
	// return EFAULT;
}


