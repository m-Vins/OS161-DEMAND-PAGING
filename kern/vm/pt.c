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
 * entry for them in order to not waste memory.
 * It means that the virtual address in the page table 
 * IS NOT computed as index * PAGE_SIZE.
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
 * only the page of the address space that belong to a segment are 
 * considered in the page table. 
 *  
 * 
 * @param as address space
 * @param vaddr virtual address
 * @return int page table index
 */
static int pt_get_index(struct addrspace *as, vaddr_t vaddr){
    unsigned int pt_index;
    
    KASSERT(as != NULL);

    switch(as_get_segment_type(as,vaddr)){
        case SEGMENT_TEXT:
            pt_index = ( vaddr - as->as_text->seg_base_vaddr ) / PAGE_SIZE;
            KASSERT(pt_index < as->as_text->seg_npages);
            return pt_index;
        case SEGMENT_DATA:
            pt_index = as->as_text->seg_npages + ( vaddr - as->as_data->seg_base_vaddr ) / PAGE_SIZE;
            KASSERT(pt_index <  as->as_text->seg_npages + as->as_data->seg_npages);
            return pt_index;
        case SEGMENT_STACK:
            pt_index = as->as_data->seg_npages + as->as_text->seg_npages + (vaddr - as->as_stack->seg_base_vaddr) / PAGE_SIZE ;
            KASSERT(pt_index <  as->as_data->seg_npages + as->as_text->seg_npages + as->as_stack->seg_npages);
            return pt_index;
        default :
            panic("vaddr out of range! (pt_get_index)\n");
    }
    
    return 0;
}

/**
 * @brief allocates the page table and initializes it.
 * 
 * @param pagetable_size 
 * @return struct pt_entry* 
 */
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
        pt[i].pt_frame_index = 0;
        pt[i].pt_swap_index = 0;
        pt[i].pt_status = NOT_LOADED;
    }

    return pt;
}

/**
 * @brief retrieve the pointer to the page table entry for the given virtual address
 * 
 * @param as 
 * @param vaddr 
 * @return struct pt_entry* 
 */
struct pt_entry *pt_get_entry(struct addrspace *as, const vaddr_t vaddr)
{
    KASSERT(as != NULL);
    
    int pt_index = pt_get_index(as, vaddr);
    
    return &as->as_ptable[pt_index];
}

/**
 * @brief deallocates the page table. Beaware of calling pt_empty before this
 * to not waste memory.
 * 
 * @param entry 
 */
void pt_destroy(struct pt_entry* entry) 
{
    KASSERT(entry != NULL);
    kfree(entry);
}

/**
 * @brief deallocates both the pages in memory and the pages 
 * in the swap file.
 * 
 * @param pt 
 * @param size 
 */
void pt_empty(struct pt_entry* pt, int size){
    paddr_t paddr;
    KASSERT(pt != NULL);
    KASSERT(pt != 0);

    for(int i = 0; i < size; i++){
        switch (pt[i].pt_status)
        {
            case IN_MEMORY:
                paddr = ( pt[i].pt_frame_index ) * PAGE_SIZE;
                free_upage(paddr);
                break;
            case IN_SWAP:
#if OPT_SWAP       
                swap_free(pt[i].pt_swap_index);
#else           
                panic("SWAP Pages should not exists!");
#endif
                break;
            default:
                break;
        }
    }

}

/**
 * @brief set the given page table entry
 * 
 * @param pt_row 
 * @param paddr 
 * @param swap_index 
 * @param status 
 */
void pt_set_entry(struct pt_entry *pt_row, paddr_t paddr, unsigned int swap_index, unsigned char status){
    
    KASSERT(status == IN_MEMORY || status == IN_SWAP || status == NOT_LOADED);
    KASSERT(swap_index < 0xfff);     /*  it should be on 12 bit */

    pt_row->pt_frame_index = paddr/PAGE_SIZE;
    pt_row->pt_swap_index = swap_index;
    pt_row->pt_status = status;

}
