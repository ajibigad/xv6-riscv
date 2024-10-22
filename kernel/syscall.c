#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
// reads from user space vmem address addr into kernel space pointer variable ip. 
// The value read from addr vmem should be another vmem address
//  This is useful for copying pointer of pointer args into kernel mem
// For example, if syscall arg 0 is char** ptr. It means arg 0 contains user vmem address A and address A contains another user vmem address B
//  So to get address B we have to get address A first by using ardadd(0, addr), addr will now contain address A
//  To get address B we then call fetchaddr(addr, ip), kernel variable pointer ip will now contains address B
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  // checks that the address is valid. 
  //   It is not greater the max address in the process memory
  //   It is at least 64 bytes before the max address so we do not read overflow i.e read beyond the max address in the memory
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;

  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
// reads from provided user space vmem address addr into kernel space buffer
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  // copyinstr => (copy in string), a type of the copyin function that copies nul-terminated string from an address in user process memory to a destination buffer in kernel memory
  if(copyinstr(p->pagetable, buf, addr, max) < 0) 
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
// Basically returns a user space virtual memory. To read/write to this address, you will need to use copyin/copyout and provide the process' pagetable
// for example if syscall is sys_info(&sysinfostruct) or sys_info(sys_info_ptr) where struct sysinfo* sys_info_ptr = &sysinfostruct, 
//   argaddr will copy the virtual address of sysinfostruct from user space into arg_addr variable in kernel space
//   We can then copy data from kernel mem space into user space by calling copyout and passing this userspace vmem address as the dest and also provide the user space process pagetable
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max); //read from user space vmem address and copy into kernel space buf
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_trace(void);
extern uint64 sys_info(void);
extern uint64 sys_pgaccess(void);
extern uint64 sys_pgdirty(void);
extern uint64 sys_sigalarm(void);
extern uint64 sys_sigreturn(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_sysinfo] sys_info,
[SYS_pgaccess] sys_pgaccess,
[SYS_pgdirty] sys_pgdirty,
[SYS_sigalarm] sys_sigalarm,
[SYS_sigreturn] sys_sigreturn,
};

static char* syscall_names[] = {
  [SYS_fork]    "fork",
  [SYS_exit]    "exit",
  [SYS_wait]    "wait",
  [SYS_pipe]    "sys_pipe",
  [SYS_read]    "read",
  [SYS_kill]    "kill",
  [SYS_exec]    "exec",
  [SYS_fstat]   "fstat",
  [SYS_chdir]   "chdir",
  [SYS_dup]     "dup",
  [SYS_getpid]  "getpid",
  [SYS_sbrk]    "sbrk",
  [SYS_sleep]   "sleep",
  [SYS_uptime]  "uptime",
  [SYS_open]    "open",
  [SYS_write]   "write",
  [SYS_mknod]   "mknod",
  [SYS_unlink]  "unlink",
  [SYS_link]    "link",
  [SYS_mkdir]   "mkdir",
  [SYS_close]   "close",
  [SYS_trace]   "trace",
  [SYS_sysinfo] "sysinfo",
  [SYS_pgaccess] "pgaccess",
  [SYS_pgdirty] "pgdirty",
  [SYS_sigalarm] "sigalarm",
  [SYS_sigreturn] "sigreturn",
};

_Static_assert(NELEM(syscalls) == NELEM(syscall_names), "syscalls and syscall_names count do not match.");

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  // Fetch syscall number/index from user process register a7
  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // Use num to lookup the system call function for num, call it,
    // and store its return value in p->trapframe->a0
    p->trapframe->a0 = syscalls[num]();

    if (p->tracemask & 1 << num) {
      printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], p->trapframe->a0);    
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
