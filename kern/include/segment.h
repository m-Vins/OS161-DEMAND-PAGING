#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>
#include "opt-rudevm.h"

#if OPT_RUDEVM

struct segment {
    off_t elf_offset;
    vaddr_t base_vaddr;     /*  address of the first page in virtual memory */
    vaddr_t first_vaddr;    /*  actual first address of the segment         */
    vaddr_t last_vaddr;     /*  last address of the segment                 */
    size_t elfsize;
    size_t npages;
};

struct segment *segment_create(void);
void segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, vaddr_t first_vaddr, vaddr_t last_vaddr, size_t npages, size_t elfsize); 
void segment_destroy(struct segment *seg);

#endif /* OPT_RUDEVM */

#endif /* _SEGMENT_H_ */
