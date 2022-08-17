#include <types.h>
#include <vm.h>
#include <lib.h>
#include <coremap.h>
#include <mainbus.h>
#include <swapfile.h>
#include <pt.h>

vaddr_t firstfree; /* first free virtual address; set by start.S */


static int nRamFrames = 0; /* number of ram frames */
struct coremap_entry *coremap;
int victim_index;


/**
 * @brief Initialization of the coremap, this function is called 
 * in the very initial phase of the system bootsrap. It replace ram_bootstrap
 * as in the version with demand paging ram.c is totally bypassed.
 *  
 */
void coremap_bootstrap(){
  paddr_t firstpaddr;   /* one past end of first free physical page */
  paddr_t lastpaddr;    /* one past end of last free physical page */
  size_t coremap_size;  /* number of bytes of coremap */
  int coremap_pages;
  int kernel_pages;
  int i;

  /* Initialize variable for victim finding */
	victim_index = 0;

  /* Get size of RAM. */
  lastpaddr = mainbus_ramsize();

  /*
   * This is the same as the last physical address, as long as
   * we have less than 512 megabytes of memory. If we had more,
   * we wouldn't be able to access it all through kseg0 and
   * everything would get a lot more complicated. This is not a
   * case we are going to worry about.
   */
  if (lastpaddr > 512 * 1024 * 1024)
  {
    lastpaddr = 512 * 1024 * 1024;
  }

  /*
   * Get first free virtual address from where start.S saved it.
   * Convert to physical address.
   */
  firstpaddr = firstfree - MIPS_KSEG0;

  KASSERT(lastpaddr % PAGE_SIZE == 0);

  nRamFrames = lastpaddr / PAGE_SIZE;


  /* Allocates the coremap right in the firstfree address. */
  coremap = (struct coremap_entry *)firstfree;
  
  /* 
   * Compute the size of coremap and kernel in pages in order to set 
   * the pages right after firstfree as used.
   */
  coremap_size = sizeof(struct coremap_entry) * nRamFrames;
  coremap_pages = (coremap_size + PAGE_SIZE - 1) / PAGE_SIZE;
  kernel_pages = firstpaddr / PAGE_SIZE;

  /*  Initialize the coremap. */
  for (i = 0; i < nRamFrames; i++)
  {
    coremap[i].allocSize = 0;
    coremap[i].used = 0;
    coremap[i].p_addrspace = NULL;
  }

  /* 
   * Set the initial part of the coremap as used by the kernel.
   * It contains the exception handlers, the kernel, the coremap and some padding.
   */
  for (i = 0; i < kernel_pages + coremap_pages; i++)
  {
    coremap[i].used = 1;
  }

}

/**
 * @brief get npages from the ram.
 * 
 * @param npages
 * @param p_addrspace
 * @return paddr_t of the pages, 0 if no pages are available.
 */
paddr_t coremap_getppages(int npages, struct addrspace *p_addrspace)
{
  int end;
  int beginning;
  int i;


  end = 0;
  beginning = -1;
  while (end < nRamFrames)
  {
    if (coremap[end].used == 1)
    {
      beginning = -1;
      end += coremap[end].used;
    }
    else // frame is free
    {
      if (beginning == -1)
      {
        beginning = end;
      }
      end++;

      if (end - beginning == npages)
        break;
    }
  }

  if (beginning == -1 || end - beginning != npages)
    return 0;

  coremap[beginning].allocSize = npages;
  for (i = 0; i < npages; i++)
  {
    coremap[beginning + i].used = 1;
    coremap[beginning + i].p_addrspace = p_addrspace;
  }
  

  return beginning * PAGE_SIZE;
}

/**
 * @brief free the allocated pages starting from addr
 * 
 * @param addr 
 */
void coremap_freeppages(paddr_t addr){
  long i,first = addr / PAGE_SIZE;
  long allocSize = coremap[first].allocSize;


  KASSERT(nRamFrames > first);
  KASSERT(allocSize != 0);
  

  for(i = first; i<allocSize; i++){
    coremap[i].used = 0;
  }

  coremap[first].allocSize = 0;

}

/**
 * @brief swap out a frame and return its physical address
 * 
 * @param p_addrspace 
 */
paddr_t
coremap_swapout(struct addrspace *p_addrspace)
{
  int i;
  int swap_index;
  struct pt_entry *old_pt_entry;

  for(i=0; i<nRamFrames; i++)
  {
    victim_index = (victim_index + 1) % nRamFrames;
    if(coremap[victim_index].p_addrspace != NULL)
    {
      KASSERT(coremap[victim_index].used == 1);
      KASSERT(coremap[victim_index].allocSize == 1);

      /* Swap out and obtain swap index */
      swap_index = swap_out(victim_index * PAGE_SIZE);

      /* Obtain the pt_entry from the old address space */
      old_pt_entry = pt_get_entry_from_paddr(coremap[victim_index].p_addrspace, victim_index * PAGE_SIZE);

      /* Remove frame index, set swap index and set as swapped out */
      old_pt_entry->frame_index = 0;
      old_pt_entry->swap_index = swap_index;
      old_pt_entry->status=IN_SWAP;

      /* Assign the new address space */
      coremap[victim_index].p_addrspace = p_addrspace;

      victim_index = (victim_index + 1) % nRamFrames;
      return victim_index * PAGE_SIZE;
    }
  }

  panic("coremap_swapout should not return");
}
