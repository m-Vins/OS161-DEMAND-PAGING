#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>

struct segment {
    off_t elf_offset;
    vaddr_t base_vaddr;
    size_t npages;

};

struct segment *segment_create(void);
void segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, size_t npages); 
void segment_destroy(struct segment *seg);


#endif 