#ifndef _PT_H_
#define _PT_H_

#include <types.h>

#define NOT_LOADED 0
#define IN_MEMORY 1
#define IN_SWAP 2

#define IS_IN_MEMORY(pt_entry) (pt_entry->status==IN_MEMORY)
#define IS_IN_SWAP(pt_entry) (pt_entry->status==IN_SWAP)
#define IS_NOT_LOADED(pt_entry) (pt_entry->status==NOT_LOADED)

struct pt_entry
{
    unsigned int frame_index : 20;
    unsigned int swap_index : 12;
    unsigned char status : 2;
};

struct pt_entry *pt_get_entry(const vaddr_t vaddr);
struct pt_entry *pt_create(unsigned long pagetable_size);

#endif
