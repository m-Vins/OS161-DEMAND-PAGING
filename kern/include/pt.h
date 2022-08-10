#ifndef _PT_H_
#define _PT_H_

#include <types.h>

struct pt_entry
{
    unsigned int frame_index : 20;
    unsigned int swap_index : 12;
};

struct pt_entry *pt_get_entry(const vaddr_t vaddr);
struct pt_entry *pt_create(unsigned long pagetable_size);

#endif
