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
  if(perm == 0)
    perm = SHM_RW;
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
  }else if (lookup >= 0) {
    release(&GLOBAL_BOOK.lock);
    if(GLOBAL_BOOK.shmid_ds[lookup].shm_segsz < size || GLOBAL_BOOK.shmid_ds[lookup].no_of_pages != noofpages)
      return -1;
    return GLOBAL_BOOK.shmid_ds[lookup].shmid;
  }else {
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
    GLOBAL_BOOK.shmid_ds[lookup].shm_perm.mode = perm;

    //GLOBAL_BOOK.shmid_ds[lookup].shm_perm      #################
    // cprintf("xxx %d\n", lookup + 1);
    //cprintf("xxxxx %d ==== %d \n", lookup, GLOBAL_BOOK.shmid_ds[lookup].shm_perm.__key);
    release(&GLOBAL_BOOK.lock);
    return lookup;
  }
  return -1;
} 

void*
shmat(int shmid, const void *shmaddr, int shmflg) {

  int perm = 0, lookup = 0, noofpages = GLOBAL_BOOK.shmid_ds[shmid].no_of_pages;
  void* vir; 

  cprintf("error in 153 \n");

  acquire(&GLOBAL_BOOK.lock);
  struct proc *curproc = myproc();

  for(int i = 0; i < MAX_REGIONS_PER_PROC; i++) {
    if (curproc->sharedmem.sharedseg[i].shmid == shmid) {
      release(&GLOBAL_BOOK.lock);
      if(shmflg == SHM_RDONLY)
        if(GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_R)
          return (char*)curproc->sharedmem.sharedseg[i].viraddr;
      if (shmflg == 0)
        if(GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_RW)
          return (char*)curproc->sharedmem.sharedseg[i].viraddr;
      cprintf("error in 165 \n");
      return (void*)-1;
    }
  }
  if(shmid < 0 || shmid > MAX_REGIONS) {
    release(&GLOBAL_BOOK.lock);
    cprintf("error in 171 \n");
    return (void*)-1;
  }
  if(shmflg == 0) {
    if (GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_RW)
      perm = PTE_W | PTE_U;
    else {
      release(&GLOBAL_BOOK.lock);
      cprintf("error in 177 \n");
      return (void*)-1;
    }
  }else if (shmflg == SHM_RDONLY) {

    if (GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_R)
      perm = PTE_U;
    else {
      release(&GLOBAL_BOOK.lock);
      cprintf("error in 186 \n");
      return (void*)-1;
    }
  }
  if(shmaddr == (void*)0)
    vir = curproc->sharedmem.next_virtual;
  else
    vir = (void*)shmaddr;

  // for(int i = 0; i < 8; i++) {
  //   if(curproc->sharedmem.sharedseg[i].key != -1) {
  //     if((int)vir + noofpages * PGSIZE  > (int)curproc->sharedmem.sharedseg[i].viraddr) {
  //       release(&GLOBAL_BOOK.lock);
  //       cprintf("errorn in line 197");
  //       return (void*) -1;
  //     }
  //   }
  // }
  for(int i = 0; i < 8; i++) {
    if(curproc->sharedmem.sharedseg[i].key == -1) {
      lookup = i;
      break;
    }
  }
  curproc->sharedmem.noofshmreg++;
  curproc->sharedmem.sharedseg[lookup].shmid = shmid;
  curproc->sharedmem.sharedseg[lookup].noofpages = noofpages;
  curproc->sharedmem.sharedseg[lookup].viraddr = vir;
  curproc->sharedmem.sharedseg[lookup].key = GLOBAL_BOOK.shmid_ds[shmid].shm_perm.__key;
  curproc->sharedmem.sharedseg[lookup].perm = GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode;

  for(int i = 0; i < noofpages; i++) {
    cprintf("%d\n", i);

    if (mappages(curproc->pgdir, vir, PGSIZE, (uint)GLOBAL_BOOK.shmid_ds[shmid].v2p[i], perm) < 0) {
      cprintf("mappages failed !");
      deallocuvm(curproc->pgdir, (uint)vir, (uint) vir + GLOBAL_BOOK.shmid_ds[shmid].shm_segsz);
      release(&GLOBAL_BOOK.lock);
      cprintf("error in 219 \n");
      return (void*)-1;
    }
    vir = vir + PGSIZE;
  }
  curproc->sharedmem.next_virtual = vir;
  //cprintf("vir to attchh = %x\n", vir);
  release(&GLOBAL_BOOK.lock);
  cprintf("error in 230 \n");

  return curproc->sharedmem.sharedseg[lookup].viraddr;
}


