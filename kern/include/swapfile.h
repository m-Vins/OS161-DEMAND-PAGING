#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>
#include "opt-swap.h"

#if OPT_SWAP

#define SWAPFILE_SIZE 9 * 1024 * 1024
#define SWAP_INDEX_SIZE 12 /* Calculated as upper(log_2(SWAPFILE_SIZE/PAGE_SIZE)) */
#define SWAPFILE_NAME "emu0:/SWAPFILE"
#define SWAPFILE_NPAGES SWAPFILE_SIZE/PAGE_SIZE

void            swap_bootstrap(void);
void            swap_in(paddr_t page_paddr, unsigned int swap_index);
unsigned int    swap_out(paddr_t page_paddr);
void            swap_free(unsigned int swap_index);
void            swap_destroy(void);

#endif /* OPT_SWAP */

#endif /* _SWAPFILE_H_ */
