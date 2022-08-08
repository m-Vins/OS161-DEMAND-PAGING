#ifndef _COREMAP_H_
#define _COREMAP_H_


struct coremap_entry
{
    unsigned int used : 1;
    unsigned int kernel : 1;
    unsigned int allocSize : 16;
};

void coremap_bootstrap(void);
paddr_t coremap_getppages(int npages, int kernel);
void coremap_freeppages(paddr_t addr);

#endif