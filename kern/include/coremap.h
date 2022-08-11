#ifndef _COREMAP_H_
#define _COREMAP_H_

#include "opt-rudevm.h"

#if OPT_RUDEVM

#define COREMAP_KERNEL 1
#define COREMAP_USER 0


struct coremap_entry
{
    unsigned int used : 1;
    unsigned int kernel : 1;
    unsigned int allocSize : 16;
};

void coremap_bootstrap(void);
paddr_t coremap_getppages(int npages, char kernel);
void coremap_freeppages(paddr_t addr);

#endif /* OPT_RUDEVM */

#endif /* _COREMAP_H_ */
