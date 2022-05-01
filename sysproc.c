#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "sys/shm.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_shmget(void) {

  int key, size;
  int shmflg;

  if(argint(0, &key) < 0 || argint(1, &size) < 0 || argint(2, &shmflg) < 0)
    return -1;

  return shmget((unsigned int)key, (unsigned int)size, shmflg);
}

void*
sys_shmat(void) {

  int shmaddr;
  int shmid, shmflg;

  if(argint(0, &shmid) < 0 || argint(1, &shmaddr) < 0 || argint(2, &shmflg) < 0)
    return (void*)-1;

  return shmat(shmid, (void*)shmaddr, shmflg);

}

int
sys_shmdt(void) {

  int shmaddr;

  if (argint(0, &shmaddr) < 0) {
    cprintf("error in fetching argumrnts");
    return -1;
  }

  return shmdt((const void*)shmaddr);
}

int
sys_shmctl(void) {

  int shmid, cmd;
  struct shmid_ds *ptr;

  if(argint(0, &shmid) < 0 || argint(1, &cmd) < 0 || argptr(2, (void*)&ptr, sizeof(*ptr)) < 0) {
    return -1;
  }

  return shmctl(shmid, cmd, ptr);
}
