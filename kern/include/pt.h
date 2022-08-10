#ifndef _PT_H_
#define _PT_H_

#include <types.h>
#include <addrspace.h>

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

struct pt_entry *pt_get_entry(const vaddr_t vaddr, struct addrspace *as);
struct pt_entry *pt_create(unsigned long pagetable_size);
int pt_set_entry(struct addrspace *as, vaddr_t vaddr, paddr_t paddr, unsigned int swap_index, unsigned char status);
void pt_destroy(struct pt_entry*);

#endif
