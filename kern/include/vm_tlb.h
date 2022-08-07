#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include <types.h>

/*
 *  tlb_invalidate: invalidate all the slots in the tlb.
 *
 *  tlb_insert: associate a vaddress to a paddress in the tlb.
 *      Set ro to true to insert as read-only.
 * 
 *  tlb_remove: remove a virtual address from the TLB if is present.
 */
void tlb_invalidate(void);
void tlb_insert(vaddr_t vaddr, paddr_t paddr, bool ro);
void tlb_remove(vaddr_t vaddr);

#endif /* _VM_TLB_H_ */