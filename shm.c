#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "sys/shm.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shmid_ds shmid_ds[MAX_REGIONS];
}GLOBAL_BOOK;

struct shminfo shminfo;

void
initshminfo() {

  shminfo.shmmax = MAX_PAGES * PGSIZE;
  shminfo.shmmin = 1;
  shminfo.shmmni = MAX_REGIONS;
  shminfo.shmseg = MAX_REGIONS_PER_PROC;
  shminfo.shmall = 64000;
}

void
initsharedmemory(void) {

  initlock(&GLOBAL_BOOK.lock, "SHM");
  acquire(&GLOBAL_BOOK.lock);

  initshminfo();
  for (int i = 0; i < MAX_REGIONS; i++) {
    GLOBAL_BOOK.shmid_ds[i].shm_perm.__key = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_perm.mode = 0;
    GLOBAL_BOOK.shmid_ds[i].shmid = i;
    GLOBAL_BOOK.shmid_ds[i].shm_segsz = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_cpid = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_lpid = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_nattch = 0;
    GLOBAL_BOOK.shmid_ds[i].no_of_pages = 0;
    for(int j = 0; j < MAX_PAGES; j++) {
      GLOBAL_BOOK.shmid_ds[i].v2p[j] = (void*)0;
    }
  }
  release(&GLOBAL_BOOK.lock);
}

int
keylookup(unsigned int key, int flag, int perm) {

  for (int i = 0; i < MAX_REGIONS; i++) {
    if (GLOBAL_BOOK.shmid_ds[i].shm_perm.__key == key) {
      if(key == 0)
        continue;
      if(flag == (IPC_CREAT | IPC_EXCL))
        return -1;
      if(perm != 0600 && perm != 0400)
        return -1;
      return i;
    }
  }
  return -2;
}

int
pagealloc(int shmid, int noofpages) {

  char *page;

  for(int i = 0; i < noofpages; i++) {
    page = kalloc();
    if(!page) {
      return 0;
    }
    memset(page, 0, PGSIZE);
    GLOBAL_BOOK.shmid_ds[shmid].v2p[i] = (void*)page;
  }
  return 1;

}

int
shmget(unsigned int key, unsigned int size, int shmflag) {

  acquire(&GLOBAL_BOOK.lock);

  int noofpages, flag, perm, lookup, success = 0;
  struct proc *proc = myproc();
  flag = shmflag & 00007000;
  perm = shmflag & 00000700;
  noofpages = size / PGSIZE;
  noofpages += 1;
  if(size <= 0 || size > KERNBASE - HEAPLIMIT || noofpages > MAX_PAGES) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  lookup = keylookup(key, flag, perm);
  if(lookup == -1) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  } else if (lookup >= 0) {
    release(&GLOBAL_BOOK.lock);
    if(GLOBAL_BOOK.shmid_ds[lookup].shm_segsz < size || GLOBAL_BOOK.shmid_ds[lookup].no_of_pages != noofpages)
      return -1;
    return GLOBAL_BOOK.shmid_ds[lookup].shmid;
  } else {
    lookup = -1;
    for(int i = 0; i < MAX_REGIONS; i++) {
      if(GLOBAL_BOOK.shmid_ds[i].shm_perm.__key == -1) {
        lookup = i;
        break;
      }
    }
    if(lookup == -1) {
      release(&GLOBAL_BOOK.lock);
      return -1;   
    }
    if(!(flag == IPC_CREAT || flag == (IPC_CREAT | IPC_EXCL)))
      return -1;
    if(!(perm == SHM_R || perm == SHM_RW || perm == SHM_W))
      return -1;
    
    success = pagealloc(lookup, noofpages);
    if(!success)
      return -1;
    
    GLOBAL_BOOK.shmid_ds[lookup].no_of_pages = noofpages;
    GLOBAL_BOOK.shmid_ds[lookup].shm_cpid = proc->pid;
    GLOBAL_BOOK.shmid_ds[lookup].shm_segsz = size;
    GLOBAL_BOOK.shmid_ds[lookup].shm_perm.__key = key;

    //GLOBAL_BOOK.shmid_ds[lookup].shm_perm      #################
    // cprintf("xxx %d\n", lookup + 1);
    cprintf("xxx %d ==== %d \n", lookup, GLOBAL_BOOK.shmid_ds[lookup].shm_perm.__key);
    release(&GLOBAL_BOOK.lock);
    return lookup;


  }






  

  return 0;






}








