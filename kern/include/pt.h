#ifndef _PT_H_
#define _PT_H_

#include <types.h>

struct pt_entry
{
    unsigned int frame_index : 20;
    unsigned int swap_index : 12;
};

struct pt_entry *get_pt_entry(const vaddr_t vaddr);

#endif
