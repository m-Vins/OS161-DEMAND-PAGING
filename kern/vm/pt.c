#include <pt.h>
#include <proc.h>
#include <addrspace.h>
#include <segment.h>
#include <current.h>
#include <kern/errno.h>


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


struct pt_entry *pt_get_entry(const vaddr_t vaddr)
{
    int pt_index;
    struct addrspace *cur_as;

    cur_as = curproc->p_addrspace;

    if (vaddr >= cur_as->s_text->base_vaddr && vaddr < cur_as->s_data->base_vaddr + cur_as->s_data->npages * PAGE_SIZE)
    {
        pt_index = vaddr / PAGE_SIZE;
        return &curproc->p_ptable[pt_index];
    }

    if (vaddr >= cur_as->s_stack->base_vaddr && vaddr < cur_as->s_stack->base_vaddr + cur_as->s_stack->npages)
    {
        pt_index = cur_as->s_text->npages + cur_as->s_data->npages + (vaddr - cur_as->s_stack->base_vaddr)/PAGE_SIZE ;
        return &curproc->p_ptable[pt_index];
    }

    return NULL;
}
