/*
 * AUthor: G.Cabodi
 * Very simple implementation of sys__exit.
 * It just avoids crash/panic. Full process exit still TODO
 * Address space is released
 */

#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <current.h>
#include <synch.h>

/*
 * simple proc management system calls
 */
void
sys__exit(int status)
{
  #if  OPT_WAITPID
  struct proc *p = curproc;
  p->status = status & 0xff;
  proc_remthread(curthread);

  V(p->p_sem);
  /* 
   * here it is not destroyed the proc structure since the 
   * process waiting for the termination of this process
   * need the exits status, hence the it is freed 
   * within the proc_wait().
   */

#else

  /* get address space of current process and destroy */
  struct addrspace *as = proc_getas();
  as_destroy(as);
  
#endif

  /* thread exits. proc data structure will be lost */
  thread_exit();

  panic("thread_exit returned (should not happen)\n");
  (void) status; // TODO: status handling
}
