#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <addrspace.h>
#include "opt-rudevm.h"

#if OPT_RUDEVM

struct coremap_entry
{
    unsigned int used : 1;
    unsigned int allocSize : 16;
    struct addrspace *p_addrspace;
};

void coremap_bootstrap(void);
paddr_t coremap_getppages(int npages, struct addrspace *p_addrspace);
void coremap_freeppages(paddr_t addr);
paddr_t coremap_swapout(struct addrspace *p_addrspace);

#endif /* OPT_RUDEVM */

#endif /* _COREMAP_H_ */
