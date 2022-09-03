#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <pt.h>
#include "opt-rudevm.h"

#if OPT_RUDEVM

struct coremap_entry
{
    unsigned char       cm_used;
    unsigned long       cm_allocsize;      
    unsigned char       cm_lock;
    struct pt_entry     *cm_ptentry;            /*  page table entry of the page living 
                                                    in this frame, NULL if kernel page  */
};

void        coremap_bootstrap(void);
paddr_t     coremap_getppages(int npages, struct pt_entry *ptentry);
void        coremap_freeppages(paddr_t addr);

#endif /* OPT_RUDEVM */

#endif /* _COREMAP_H_ */
