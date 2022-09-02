#ifndef _VMSTATS_H_
#define _VMSTATS_H_

#define VMSTAT_TLB_FAULT 0
#define VMSTAT_TLB_FAULT_FREE 1
#define VMSTAT_TLB_FAULT_REPLACE 2
#define VMSTAT_TLB_INVALIDATION 3
#define VMSTAT_TLB_RELOAD 4
#define VMSTAT_PAGE_FAULT_ZERO 5
#define VMSTAT_PAGE_FAULT_DISK 6
#define VMSTAT_PAGE_FAULT_ELF 7
#define VMSTAT_PAGE_FAULT_SWAP 8
#define VMSTAT_SWAP_WRITE 9

void vmstats_hit(unsigned int stat);
void vmstats_print(void);

#endif