#include <segment.h>
#include <lib.h>


struct segment *segment_create(void){
    struct segment *seg = kmalloc(sizeof(struct segment));

    KASSERT(seg != NULL);

    seg->elf_offset = 0;
    seg->base_vaddr = 0;
    seg->npages = 0;
    
    return seg;
}


void segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, size_t npages){

    KASSERT(seg != NULL);
    KASSERT(seg->elf_offset == 0);
    KASSERT(seg->base_vaddr == 0);
    KASSERT(seg->npages == 0);

    seg->elf_offset = elf_offset;
    seg->base_vaddr = base_vaddr;
    seg->npages = npages;

}


void segment_destroy(struct segment *seg){
    
    KASSERT(seg != NULL);

    kfree(seg);
}