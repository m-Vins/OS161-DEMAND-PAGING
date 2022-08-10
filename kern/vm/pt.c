#include <pt.h>
#include <proc.h>
#include <addrspace.h>
#include <segment.h>
#include <current.h>
#include <kern/errno.h>

/**
 * @brief Compute the page table index of the virtual address.
 * 
 * Check the segment from which the vaddr belongs to. 
 * In case it belongs either to data or text segment, the page table 
 * index is computed by means of a division.
 * 
 * Whereas, if the vaddr belongs to the stack segment, it must be taken 
 * into account the logic behind the page table as it does not have an 
 * entry for each page of the virtual address space. Indeed, the free 
 * region between the stack and the data segments is totally skipped
 * in the array of the page table entries. Thus, the number of skipped
 * pages must be substracted to compute the index in the page table.
 * 
 *  _________________
 * | stack           |
 * |_________________|
 * |                 |
 * |                 |
 * | free            |
 * |_________________|
 * | data            |
 * |_________________|
 * | text            |
 * |_________________|
 * 
 * 
 * @param as address space
 * @param vaddr virtual address
 * @return int page table index
 */
static int pt_get_index(struct addrspace *as,vaddr_t vaddr){
    int pt_index;

    if (vaddr >= as->s_text->base_vaddr && vaddr < as->s_data->base_vaddr + as->s_data->npages * PAGE_SIZE)
    {
        pt_index = vaddr / PAGE_SIZE;
        return pt_index;
    }

    if (vaddr >= as->s_stack->base_vaddr && vaddr < as->s_stack->base_vaddr + as->s_stack->npages)
    {
        pt_index = as->s_text->npages + as->s_data->npages +  (vaddr - as->s_stack->base_vaddr) / PAGE_SIZE ;
        return pt_index;
    }

    panic("vaddr out of range?!\n");
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
    }

    return pt;
}


struct pt_entry *pt_get_entry(const vaddr_t vaddr, struct addrspace *as)
{
    
    int pt_index = pt_get_index(as, vaddr);
    
    return &as->as_ptable[pt_index];
}

int pt_set_entry(struct addrspace *as, vaddr_t vaddr, paddr_t paddr, unsigned int swap_index, unsigned char status){
    
    KASSERT( (paddr == 0 && swap_index != 0 && status == IN_SWAP) || (paddr != 0 && swap_index == 0 && status == IN_MEMORY) );

    struct pt_entry *entry = pt_get_entry(vaddr, as);
    if(entry == NULL){
        return -1;
    }    

    entry->frame_index = (unsigned int)(paddr_t>>12);
    entry->swap_index = swap_index;
    entry->status = status;

    return 1; 

}

void pt_destroy(struct pt_entry*)
{
    KASSERT(pt_entry != NULL);
    kfree(pt_entry);
}

