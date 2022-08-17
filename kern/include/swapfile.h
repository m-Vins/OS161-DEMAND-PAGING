#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>
#include "opt-rudevm.h"

#if OPT_RUDEVM

#define SWAPFILE_SIZE 9 * 1024 * 1024
#define SWAPFILE_NAME "emu0:/SWAPFILE"

void swap_bootstrap(void);
void swap_in(paddr_t page_paddr, unsigned int swap_index);
unsigned int swap_out(paddr_t page_paddr);

#endif /* OPT_RUDEVM */

#endif /* _SWAPFILE_H_ */
