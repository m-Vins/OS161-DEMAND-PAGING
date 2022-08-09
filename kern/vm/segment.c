


#include <segment.h>


struct segment *segment_create(void){
    struct segment *seg = kmalloc(sizeof(struct segment));
    seg->elf_offset = 0;
    seg->base_vaddr = 0;
    seg->npages = 0;
}


void segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, size_t npages){

    seg->elf_offset = elf_offset;
    seg->base_vaddr = base_vaddr;
    seg->npages = npages;

}