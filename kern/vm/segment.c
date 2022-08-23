#include <segment.h>
#include <lib.h>


struct segment *segment_create(void){
    struct segment *seg = kmalloc(sizeof(struct segment));

    KASSERT(seg != NULL);

    seg->elf_offset = 0;
    seg->base_vaddr = 0;
    seg->first_vaddr = 0;
    seg->last_vaddr = 0;
    seg->npages = 0;
    seg->elfsize = 0;
    
    return seg;
}


void segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, vaddr_t first_vaddr, vaddr_t last_vaddr, size_t npages, size_t elfsize){

    KASSERT(seg != NULL);
    KASSERT(seg->elf_offset == 0);
    KASSERT(seg->base_vaddr == 0);
    KASSERT(seg->first_vaddr == 0);
    KASSERT(seg->last_vaddr == 0);
    KASSERT(seg->npages == 0);
    KASSERT(seg->elfsize == 0);

    seg->elf_offset = elf_offset;
    seg->base_vaddr = base_vaddr;
    seg->first_vaddr = first_vaddr;
    seg->last_vaddr = last_vaddr;
    seg->npages = npages;
    seg->elfsize = elfsize;
}


void segment_destroy(struct segment *seg){
    
    KASSERT(seg != NULL);

    kfree(seg);
}