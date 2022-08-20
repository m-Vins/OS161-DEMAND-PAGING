#include <swapfile.h>
#include <bitmap.h>
#include <kern/fcntl.h>
#include <uio.h>
#include <vfs.h>
#include <vm.h>
#include <vnode.h>

static struct vnode *swapfile;
static struct bitmap *swapmap;

void swap_bootstrap(void)
{
    int err;
    char swapfile_name[16];

    KASSERT(SWAPFILE_SIZE % PAGE_SIZE == 0);

    strcpy(swapfile_name, SWAPFILE_NAME);
    err = vfs_open(swapfile_name, O_RDWR | O_CREAT, 0, &swapfile);
    if (err)
    {
        panic("Cannot open SWAPFILE");
    }

    swapmap = bitmap_create(SWAPFILE_SIZE / PAGE_SIZE);
}

void swap_in(paddr_t page_paddr, unsigned int swap_index)
{
    int err;
    off_t swap_offset;
    struct iovec iov;
    struct uio ku;

    KASSERT(page_paddr % PAGE_SIZE == 0);
    KASSERT(swap_index < SWAPFILE_SIZE / PAGE_SIZE);
    KASSERT(bitmap_isset(swapmap, swap_index));

    swap_offset = swap_index * PAGE_SIZE;

    uio_kinit(&iov, &ku, (void *)PADDR_TO_KVADDR(page_paddr), PAGE_SIZE, swap_offset, UIO_READ);
    err = VOP_READ(swapfile, &ku);
    if (err)
    {
        panic("Error swapping in\n");
    }

    bitmap_unmark(swapmap, swap_index);
}

unsigned int swap_out(paddr_t page_paddr)
{
    int err;
    unsigned int swap_index;
    off_t swap_offset;
    struct iovec iov;
    struct uio ku;

    KASSERT(page_paddr % PAGE_SIZE == 0);

    err = bitmap_alloc(swapmap, &swap_index);
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

void swap_free(unsigned int swap_index)
{
    bitmap_unmark(swapmap, swap_index);
}
