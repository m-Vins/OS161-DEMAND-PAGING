#include <types.h>
#include <synch.h>
#include <spl.h>
#include <lib.h>
#include <vmstats.h>

static int vmstats[10];
static struct spinlock vmstats_l = SPINLOCK_INITIALIZER;

static const char *vmstats_names[] = {
    "TLB Faults",
    "TLB Faults with Free",
    "TLB Faults with Replace",
    "TLB Invalidations",
    "TLB Reloads",
    "Page Faults (Zeroed)",
    "Page Faults (Disk)",
    "Page Faults from ELF",
    "Page Faults from Swapfile",
    "Swapfile Writes"};

void vmstats_hit(unsigned int stat)
{
    spinlock_acquire(&vmstats_l);

    KASSERT(stat < 10);
    vmstats[stat]++;

    spinlock_release(&vmstats_l);
}

void vmstats_print()
{
    kprintf("---------------------------\n");
    kprintf("VM STATS\n");
    kprintf("---------------------------\n");
    for (int i = 0; i < 10; i++)
    {
        kprintf("%s: %d\n", vmstats_names[i], vmstats[i]);
    }
    kprintf("---------------------------\n");
}