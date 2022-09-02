#include <vm_tlb.h>
#include <mips/tlb.h>
#include <spl.h>
#include <vm.h>
#include <lib.h>

int tlb_victim = 0;

void tlb_invalidate(void)
{
    int spl, i;

    /* Disable interrupts on this CPU while frobbing the TLB. */
    spl = splhigh();

    for (i = 0; i < NUM_TLB; i++)
    {
        tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    splx(spl);
}

void tlb_insert(vaddr_t vaddr, paddr_t paddr, bool ro)
{
    int spl;
    uint32_t ehi, elo;

    /* Make sure it's page-aligned */
    KASSERT((paddr & PAGE_FRAME) == paddr);

    /* Disable interrupts on this CPU while frobbing the TLB. */
    spl = splhigh();

    ehi = vaddr;
    elo = paddr | TLBLO_VALID;
    if (!ro)
    {
        elo = elo | TLBLO_DIRTY;
    }
    tlb_write(ehi, elo, tlb_victim);
    tlb_victim = (tlb_victim + 1) % NUM_TLB;

    splx(spl);
}

void tlb_remove_by_vaddr(vaddr_t vaddr)
{
    int index;
    //TODO REMOVE IT

    index = tlb_probe(vaddr, 0);
    if (index >= 0)
        tlb_write(TLBHI_INVALID(index), TLBLO_INVALID(), index);
}

void tlb_remove_by_paddr(paddr_t paddr) {
    
    KASSERT(paddr % PAGE_SIZE == 0);

    for (int i = 0; i < NUM_TLB; i++) {
        uint32_t ehi, elo;
        tlb_read(&ehi, &elo, i);
        if (paddr == (elo & PAGE_FRAME)) {
            tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
            return;
        }
    }
}