#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>

struct segment {
    off_t elf_offset;
    vaddr_t base_vaddr;
    size_t npages;

};


#endif 