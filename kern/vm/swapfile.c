#include <swapfile.h>
#include <bitmap.h>
#include <kern/fcntl.h>
#include <uio.h>
#include <vfs.h>
#include <synch.h>
#include <vm.h>
#include <vnode.h>
#include "opt-stats.h"
#if OPT_STATS
#include <vmstats.h>
#endif

static struct vnode *swapfile;
static struct bitmap *swapmap;
static struct spinlock swaplock = SPINLOCK_INITIALIZER;


/**
 * @brief creates the swap file and allocated the data
 * structures needed
 * 
 */
void swap_bootstrap(void)
{
    int err;
    char swapfile_name[] = SWAPFILE_NAME;

    KASSERT(SWAPFILE_SIZE % PAGE_SIZE == 0);

    err = vfs_open(swapfile_name, O_RDWR | O_CREAT, 0, &swapfile);
    if (err)
    {
        panic("Cannot open SWAPFILE");
    }

    swapmap = bitmap_create(SWAPFILE_NPAGES);
}

/**
 * @brief move a page from the swap file to memory at page_paddr 
 * physical address
 * 
 * @param page_paddr 
 * @param swap_index 
 */
void swap_in(paddr_t page_paddr, unsigned int swap_index)
{
    int err;
    off_t swap_offset;
    struct iovec iov;
    struct uio ku;

#if OPT_STATS
    vmstats_hit(VMSTAT_PAGE_FAULT_DISK);
    vmstats_hit(VMSTAT_SWAP_WRITE);
#endif

    KASSERT(page_paddr % PAGE_SIZE == 0);
    KASSERT(swap_index < SWAPFILE_NPAGES);
    spinlock_acquire(&swaplock);
    KASSERT(bitmap_isset(swapmap, swap_index));
    spinlock_release(&swaplock);

    swap_offset = swap_index * PAGE_SIZE;

    uio_kinit(&iov, &ku, (void *)PADDR_TO_KVADDR(page_paddr), PAGE_SIZE, swap_offset, UIO_READ);
    err = VOP_READ(swapfile, &ku);
    if (err)
    {
        panic("Error swapping in\n");
    }

    if (ku.uio_resid != 0) {
		panic("SWAP: short read on page");
	}

    spinlock_acquire(&swaplock);
    bitmap_unmark(swapmap, swap_index);
    spinlock_release(&swaplock);
}

/**
 * @brief move a page from memory to swap file and
 * return the index within the swapfile
 * 
 * @param page_paddr 
 * @return unsigned int index of the page within the swap file
 */
unsigned int swap_out(paddr_t page_paddr)
{
    int err;
    unsigned int swap_index;
    off_t swap_offset;
    struct iovec iov;
    struct uio ku;

#if OPT_STATS
    vmstats_hit(VMSTAT_PAGE_FAULT_SWAP);
#endif

    KASSERT(page_paddr % PAGE_SIZE == 0);

    spinlock_acquire(&swaplock);
    err = bitmap_alloc(swapmap, &swap_index);
    spinlock_release(&swaplock);
    if (err)
    {
        panic("Out of swap space\n");
    }

    swap_offset = swap_index * PAGE_SIZE;

    uio_kinit(&iov, &ku, (void *)PADDR_TO_KVADDR(page_paddr), PAGE_SIZE, swap_offset, UIO_WRITE);
    err = VOP_WRITE(swapfile, &ku);
    if (err)
    {
        panic("Error swapping out\n");
    }

    return swap_index;
}

/**
 * @brief free the given entry of the swap file. 
 * This function only mark the swap page as not used,
 * not any actual deallocation is done .
 * 
 * @param swap_index 
 */ 
void swap_free(unsigned int swap_index)
{
    spinlock_acquire(&swaplock);
    bitmap_unmark(swapmap, swap_index);
    spinlock_release(&swaplock);
}

//TODO add swap destroy??
