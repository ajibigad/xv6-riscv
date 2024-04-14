#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

/* Syscalls for managing processes*/

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;

  argint(0, &mask);
  trace(mask);
  return 0;
}

uint64 
sys_info(void) {
  struct sysinfo info = {0};
  uint64 arg_addr;

  argaddr(0, &arg_addr); // get the address passed as arg 0 to the syscall and save it in kernel mem space variable arg_addr

  info.nproc = nproc();
  info.freemem = kfreemem();

  // write struct sysinfo bytes into virtual address gotten from user space 
  return copyout(myproc()->pagetable, arg_addr, (char*)&info, sizeof(info));
}

uint64
sys_pgaccess(void) {
  uint64 page_start, page_end, curr_page;
  uint64 result_addr;
  int numpages, result = 0;
  pte_t* pte;
  pagetable_t pagetable;
  int i = 0;

  argaddr(0, &page_start);
  argint(1, &numpages);
  argaddr(2, &result_addr);

  page_start = PGROUNDDOWN(page_start);

  // ensure max num of pages that can be checked
  if (numpages > 1024) {
    return -1;
  }

  page_end = page_start + ((numpages - 1) * PGSIZE);

  // ensure we do not check pages after MAXVA
  if (page_end > MAXVA) {
    return -2;
  }

  curr_page = page_start;
  pagetable = myproc()->pagetable;

  for (; i < numpages; i++) {
    pte = walk(pagetable, curr_page, 0);
    curr_page += PGSIZE;

    if (pte == 0) { ///points to nothing so deferencing in the next if statement should not be allowed
      continue;
    }

    if (*pte & PTE_A) { //maybe only check if it has been accessed?
      result = result | (1 << i);
      *pte = *pte & ~PTE_A; //clear access bit
    }
  }
  
  copyout(pagetable, result_addr, (char*) &result, sizeof(uint));
  return 0;
}
