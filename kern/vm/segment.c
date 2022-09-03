#include <segment.h>
#include <lib.h>
#include <vm.h>

/**
 * @brief allocates and initializes the segment data structure
 * 
 * @return struct segment* 
 */
struct segment *segment_create(void){
    struct segment *seg = kmalloc(sizeof(struct segment));

    KASSERT(seg != NULL);

    seg->seg_elf_offset = 0;
    seg->seg_first_vaddr = 0;
    seg->seg_last_vaddr = 0;
    seg->seg_npages = 0;
    seg->seg_elf_size = 0;
    
    return seg;
}

/**
 * @brief set up the given segment
 * 
 * @param seg 
 * @param elf_offset 
 * @param base_vaddr 
 * @param first_vaddr 
 * @param last_vaddr 
 * @param npages 
 * @param elfsize 
 */
void segment_define(struct segment *seg, off_t elf_offset, vaddr_t base_vaddr, vaddr_t first_vaddr, vaddr_t last_vaddr, size_t npages, size_t elfsize){

    /*  check the segment has been correctly allocated and initialized    */
    KASSERT(seg != NULL);
    KASSERT(seg->seg_elf_offset == 0);
    KASSERT(seg->seg_first_vaddr == 0);
    KASSERT(seg->seg_last_vaddr == 0);
    KASSERT(seg->seg_npages == 0);
    KASSERT(seg->seg_elf_size == 0);

    /*  check the addresses belong to the user address space   */
    KASSERT(base_vaddr < USERSPACETOP);
    KASSERT(first_vaddr < USERSPACETOP);
    KASSERT(last_vaddr <= USERSPACETOP);


    if (elfsize > (last_vaddr - first_vaddr)) {
        kprintf("ELF: warning: segment filesize > segment memsize\n");
        elfsize = last_vaddr - first_vaddr;
	}

    seg->seg_elf_offset = elf_offset;
    seg->seg_first_vaddr = first_vaddr;
    seg->seg_last_vaddr = last_vaddr;
    seg->seg_npages = npages;
    seg->seg_elf_size = elfsize;
}

/**
 * @brief deallocates the given segment
 * 
 * @param seg 
 */
void segment_destroy(struct segment *seg){
    
    KASSERT(seg != NULL);

    kfree(seg);
}