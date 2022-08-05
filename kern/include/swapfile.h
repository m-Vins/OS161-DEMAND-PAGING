#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>

#define SWAPFILE_SIZE 9 * 1024 * 1024
#define SWAPFILE_NAME "emu0:/SWAPFILE"

void swap_init(void);
void swap_in(paddr_t page_paddr, unsigned int swap_index);
unsigned int swap_out(paddr_t page_paddr);

#endif /* _SWAPFILE_H_ */
