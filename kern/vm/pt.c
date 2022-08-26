#include <pt.h>
#include <proc.h>
#include <addrspace.h>
#include <segment.h>
#include <current.h>
#include <kern/errno.h>
#include <swapfile.h>
#include <vm.h>
#include "opt-swap.h"

/**
 * The page table is an array of entries where each of them 
 * contains information for the location of the physical page:
 * it can be in memory, in the swap file, and it can be still 
 * not loaded, thus we have to retrieve it from the elf file 
 * (this is the reason why we also save the elf offset for each 
 * segment) .
 * 
 * Since the address space contains a large number of pages 
 * which don't belong to a segment, the page table does not have an
 * entry for them in order to save memory.
 * It means that the virtual address in the page table 
 * IS NOT index * PAGE_SIZE.
 * 
 * We can think the page table as divided in three different section, 
 * each of them dedicated to one segment, and starting from the virtual
 * address we want to translate, we have to exploit the information
 * in the address space structure to discover the segment from which the
 * address belongs to, and then substract the base_vaddr and the number
 * of page dedicated to the previous segments in the page table to retrieve 
 * the index.
 * 
 * 
 */




/**
 * @brief Compute the page table index of the virtual address.
 * 
 * Check the segment from which the vaddr belongs to. 
 * 
 * It has to take into account the logic behind the page table, as 
 * only the page of the address spages that belong to a segment are 
 * considered in the page table. 
 *  
 * 
 * @param as address space
 * @param vaddr virtual address
 * @return int page table index
 */
static int pt_get_index(struct addrspace *as, vaddr_t vaddr){
    unsigned int previous_pages = 0;
    unsigned int pt_index;
    
    KASSERT(as != NULL);

    if (vaddr >= as->s_text->first_vaddr && vaddr < as->s_text->last_vaddr)
    {
        pt_index = ( vaddr - as->s_text->base_vaddr ) / PAGE_SIZE;
        KASSERT(pt_index < as->s_text->npages);
        return pt_index;
    }
    previous_pages += as->s_text->npages;

    if (vaddr >= as->s_data->first_vaddr && vaddr < as->s_data->last_vaddr)
    {
        pt_index = previous_pages + ( vaddr - as->s_data->base_vaddr ) / PAGE_SIZE;
        KASSERT(pt_index <  previous_pages + as->s_data->npages);
        return pt_index;
    }
    previous_pages += as->s_data->npages;

    if (vaddr >= as->s_stack->first_vaddr && vaddr < as->s_stack->last_vaddr)
    {
        pt_index = previous_pages + (vaddr - as->s_stack->base_vaddr) / PAGE_SIZE ;
        KASSERT(pt_index <  previous_pages + as->s_stack->npages);
        return pt_index;
    }

    panic("vaddr out of range! (pt_get_index)\n");
    return 0;
}

struct pt_entry *pt_create(unsigned long pagetable_size)
{
    unsigned long i = 0;

    struct pt_entry *pt = kmalloc(sizeof(struct pt_entry) * pagetable_size);

    if (pt == NULL)
    {
        return NULL;
    }


    for (i = 0; i < pagetable_size; i++)
    {
        pt[i].frame_index = 0;
        pt[i].swap_index = 0;
        pt[i].status = NOT_LOADED;
    }

    return pt;
}


struct pt_entry *pt_get_entry(struct addrspace *as, const vaddr_t vaddr)
{
    KASSERT(as != NULL);
    
    int pt_index = pt_get_index(as, vaddr);
    
    return &as->as_ptable[pt_index];
}

void pt_destroy(struct pt_entry* entry) 
{
    KASSERT(entry != NULL);
    kfree(entry);
}

void pt_empty(struct pt_entry* pt, int size){
    paddr_t paddr;
    KASSERT(pt != NULL);
    KASSERT(pt != 0);

    for(int i = 0; i < size; i++){
        switch (pt[i].status)
        {
            case IN_MEMORY:
                paddr = ( pt[i].frame_index ) * PAGE_SIZE;
                free_upage(paddr);
                break;
#if OPT_SWAP
            case IN_SWAP:
                swap_free(pt[i].swap_index);
#endif
                break;
            default:
                break;
        }
    }

}

struct pt_entry *
pt_get_entry_from_paddr(struct addrspace *as, const paddr_t paddr)
{
    int i;
    int n;
    unsigned int frame_index;

    frame_index = paddr & PAGE_FRAME;
    n = as->s_data->npages + as->s_text->npages + as->s_stack->npages;
    for(i=0; i<n; i++)
    {
        if(as->as_ptable[i].frame_index == frame_index)
        {
            return &as->as_ptable[i];
        }
    }

    return NULL;
}
