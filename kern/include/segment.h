#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>
#include "opt-rudevm.h"

#if OPT_RUDEVM

struct segment {
    vaddr_t     seg_base_vaddr;     /*  address of the first page in virtual memory */
    vaddr_t     seg_first_vaddr;    /*  actual first address of the segment         */
    vaddr_t     seg_last_vaddr;     /*  last address of the segment                 */
    size_t      seg_elf_size;       /*  size of the segment within the elf          */
    off_t       seg_elf_offset;     /*  offset of the segment within the elf        */
    size_t      seg_npages;         /*  size of the segment in pages                */
};

struct segment *segment_create(void);
void            segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, vaddr_t first_vaddr, vaddr_t last_vaddr, size_t npages, size_t elfsize); 
void            segment_destroy(struct segment *seg);

#endif /* OPT_RUDEVM */

#endif /* _SEGMENT_H_ */
