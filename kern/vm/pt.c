#include <pt.h>
#include <proc.h>
#include <addrspace.h>
#include <current.h>

struct pt_entry *get_pt_entry(const vaddr_t vaddr)
{
    int page_index;
    struct addrspace *cur_as;

    page_index = vaddr / PAGE_SIZE;

    cur_as = curproc->p_addrspace;

    if (page_index >= cur_as->s_text->vbasepindex && page_index < cur_as->s_text->vbasepindex + cur_as->s_text->npages)
    {
        return curproc->p_ptable[page_index];
    }

    if (page_index >= cur_as->s_data->vbasepindex && page_index < cur_as->s_data->vbasepindex + cur_as->s_data->npages)
    {
        page_index = page_index - cur_as->s_text->npages;
        return curproc->p_ptable[page_index];
    }

    if (page_index >= cur_as->s_stack->vbasepindex && page_index < cur_as->s_stack->vbasepindex + cur_as->s_stack->npages)
    {
        page_index = page_index - cur_as->s_text->npages - cur_as->s_data->npages;
        return curproc->p_ptable[page_index];
    }
}
