#ifndef _PT_H_
#define _PT_H_

#include <types.h>
#include <addrspace.h>
#include "opt-rudevm.h"
#include "opt-noswap_rdonly.h"
#include "opt-swap.h"
#include <swapfile.h>

#if OPT_RUDEVM

#define NOT_LOADED 0
#define IN_MEMORY 1
#define IN_SWAP 2
#define IN_MEMORY_RDONLY 3



struct pt_entry
{
    unsigned int    pt_frame_index : 20;
#if OPT_SWAP
    unsigned int    pt_swap_index : SWAP_INDEX_SIZE;
#else
    unsigned int    pt_swap_index : 12;
#endif
    unsigned char   pt_status : 2;
};

struct pt_entry     *pt_get_entry(struct addrspace *as, const vaddr_t vaddr);
struct pt_entry     *pt_create(unsigned long pagetable_size);
void                pt_empty(struct pt_entry* pt, int size);
void                pt_destroy(struct pt_entry*);
void                pt_set_entry(struct pt_entry *pt_row, paddr_t paddr, unsigned int swap_index, unsigned char status);

#endif /* OPT_RUDEVM */

#endif /* _PT_H_ */
