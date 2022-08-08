/**
 * @file coremap.c
 * @author Vincenzo Mezzela
 * @brief it keeps track of free and used physical ram frames.
 * @version 0.1
 * @date 2022-08-03
 * 
 */


#include <types.h>
#include <vm.h>
#include <lib.h>
#include <coremap.h>
#include <mainbus.h>

vaddr_t firstfree; /* first free virtual address; set by start.S */


static int nRamFrames = 0; /* number of ram frames */
struct coremap_entry *coremap;


/**
 * @brief 
 * 
 */
void coremap_bootstrap(){

  paddr_t lastpaddr;  /* one past end of last free physical page */
  size_t coremap_size; /* number of bytes of coremap */
  int coremap_pages;
  int i;

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

  KASSERT(lastpaddr % PAGE_SIZE == 0);

  nRamFrames = lastpaddr / PAGE_SIZE;


  /* Allocates the coremap right in the firstfree address. */
  coremap = (struct coremap_entry *)firstfree;
  
  /* 
   * Compute the size of the coremap in pages in order to set 
   * the pages right after firstfree as used.
   */
  coremap_size = sizeof(struct coremap_entry) * nRamFrames;
  coremap_pages = (coremap_size + PAGE_SIZE - 1) / PAGE_SIZE;

  /*  Initialize the coremap. */
  for (i = 0; i < nRamFrames; i++)
  {
    coremap[i].allocSize = 0;
    coremap[i].used = 0;
    coremap[i].kernel = 0;
  }

  /* it is set to 0 as we do not need to free this part of the memory */
  coremap[0].allocSize = 0; 
  /* 
   * Set the initial part of the coremap as used by the kernel.
   * It contains the exception handlers, the kernel, the coremap and some padding.
   */
  for (i = 0; i < coremap_pages + (long)firstfree; i++)
  {
    coremap[i].used = 1;
    coremap[i].kernel = 1;
  }

}

/**
 * @brief get npages from the ram.
 * 
 * @param npages 
 * @return paddr_t of the pages, 0 if no pages are available.
 */
paddr_t coremap_getppages(int npages, int kernel)
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
    coremap[beginning + i].kernel = kernel;
  }
  

  return beginning * PAGE_SIZE;
}

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


