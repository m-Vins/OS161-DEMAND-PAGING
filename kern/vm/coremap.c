/**
 * @file coremap.c
 * @author Vincenzo Mezzela
 * @brief it keeps track of free and used physical ram frames.
 * @version 0.1
 * @date 2022-08-03
 * 
 */


#include <spinlock.h>
#include <vm.h>
#include <lib.h>



static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;
static struct spinlock mem_lock = SPINLOCK_INITIALIZER;

static int nRamFrames = 0;
static unsigned long *allocSize = NULL;

/*NOTE: we can use a bitmap as well, this is just an initial implementation */
static unsigned char *freeRamFrames = NULL;

char coremapActive = 0;

/**
 * @brief check whether the coremap is active
 * 
 * @return 1 if the coremap is active, 0 if not.
 */
static int isCoremapActive () {
  int active;
  spinlock_acquire(&coremap_lock);
  active = coremapActive;
  spinlock_release(&coremap_lock);
  return active;
}


/**
 * @brief activate the coremap, i.e. allocates the data structures and
 * initialize them.
 * 
 */
void coremap_init(){

    nRamFrames = ((int)ram_getsize())/PAGE_SIZE;
    freeRamFrames = kmalloc(sizeof(unsigned char)*nRamFrames);
    
    if (freeRamFrames == NULL) return;  
    
    allocSize = kmalloc(sizeof(unsigned long)*nRamFrames);
    
    if (allocSize == NULL) {    
        freeRamFrames = NULL; 
        return;
    }

    for (i=0; i<nRamFrames; i++) {    
        freeRamFrames[i] = (unsigned char)0;
        allocSize[i] = 0;  
    }
    spinlock_acquire(&freemem_lock);
    allocTableActive = 1;
    spinlock_release(&freemem_lock);

};

/**
 * @brief deactivate and free coremap.
 * 
 */
void coremap_destroy(){
    
    spinlock_acquire(&coremap_lock);
    coremapActive = 0;
    spinlock_release(&coremap_lock);
    
    kfree(freeRamFrames);
    kfree(allocSize);
}

/**
 * @brief get npages free pages.
 * 
 * @param npages number of pages needed.
 * @return paddr_t the starting physical address.
 */
static paddr_t 
getfreeppages(unsigned long npages) {
  paddr_t addr;	
  long i, first, found, np = (long)npages;

  if (!isCoremapActive()) return 0; 

  spinlock_acquire(&mem_lock);

  for (i=0, first = found =-1; i<nRamFrames; i++) {
    if (freeRamFrames[i]) {
      if (i==0 || !freeRamFrames[i-1]) 
        first = i; /* set first free in an interval */   
      if (i-first+1 >= np) {
        found = first;
        break;
      }
    }
  }
	
  if (found>=0) {
    for (i=found; i<found+np; i++) {
      freeRamFrames[i] = (unsigned char)0;
    }
    allocSize[found] = np;
    addr = (paddr_t) found*PAGE_SIZE;
  }
  else {
    addr = 0;
  }

  spinlock_release(&mem_lock);

  return addr;
}

/**
 * @brief get npages free pages. if there are not enogugh free pages it will 
 * steal memory from the ram.
 * 
 * please note that is is only called by the kernel, since the user can only allocate
 * one page at a time.
 * 
 * @param npages number of pages needed.
 * @return paddr_t the starting physical address.
 */
static paddr_t
getppages(unsigned long npages)
{
  paddr_t addr;

  /* try freed pages first */
  addr = getfreeppages(npages);
  if (addr == 0) {
    /* call stealmem */
    spinlock_acquire(&stealmem_lock);
    addr = ram_stealmem(npages);
    spinlock_release(&stealmem_lock);
  }
  if (addr!=0 && isTableActive()) {
    spinlock_acquire(&freemem_lock);
    allocSize[addr/PAGE_SIZE] = npages;
    spinlock_release(&freemem_lock);
  } 

  return addr;
}


/**
 * @brief allocate kernel space.
 * 
 * @param npages 
 * @return vaddr_t 
 */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	dumbvm_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

/**
 * @brief deallocate kernel space.
 * 
 * @param addr 
 */
void 
free_kpages(vaddr_t addr){
  if (isCoremapActive()) {
    paddr_t paddr = addr - MIPS_KSEG0;
    long first = paddr/PAGE_SIZE;	
    KASSERT(allocSize!=NULL);
    KASSERT(nRamFrames>first);
    freeppages(paddr, allocSize[first]);	
  }
}

/**
 * @brief allocate a page for the user. 
 * It is different from the alloc_kpage as it allocate one frame at a time and
 * has to manage the victim selection.
 * 
 * @return vaddr_t the virtual address of the allocated frame
 */
paddr_t 
alloc_upage(){
    //TODO manage the case in which there are no free frames with swapping.
    dumbvm_cansleep();
    return getppages(1);
}

/**
 * @brief deallocate the given page for the user.
 * 
 * @param addr 
 */
void free_upage(vaddr_t addr){
    getfreeppages(1);
};