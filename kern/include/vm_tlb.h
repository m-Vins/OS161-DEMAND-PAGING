#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include <types.h>
#include "opt-rudevm.h"

#if OPT_RUDEVM

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
void tlb_remove_by_vaddr(vaddr_t vaddr);
void tlb_remove_by_paddr(paddr_t paddr);

#endif /* OPT_RUDEVM */

#endif /* _VM_TLB_H_ */
