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
initshminfo(int i) {

  GLOBAL_BOOK.shmid_ds[i].shminfo.shmmax = MAX_PAGES * PGSIZE;
  GLOBAL_BOOK.shmid_ds[i].shminfo.shmmin = 1;
  GLOBAL_BOOK.shmid_ds[i].shminfo.shmmni = MAX_REGIONS;
  GLOBAL_BOOK.shmid_ds[i].shminfo.shmseg = MAX_REGIONS_PER_PROC;
  GLOBAL_BOOK.shmid_ds[i].shminfo.shmall = 64000;
}

void
initsharedmemory(void) {

  initlock(&GLOBAL_BOOK.lock, "SHM");
  acquire(&GLOBAL_BOOK.lock);

  for (int i = 0; i < MAX_REGIONS; i++) {
    GLOBAL_BOOK.shmid_ds[i].shm_perm.__key = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_perm.mode = 0;
    GLOBAL_BOOK.shmid_ds[i].shmid = i;
    GLOBAL_BOOK.shmid_ds[i].shm_segsz = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_cpid = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_lpid = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_nattch = 0;
    GLOBAL_BOOK.shmid_ds[i].no_of_pages = 0;
    GLOBAL_BOOK.shmid_ds[i].isdelete = 0;
    initshminfo(i);
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
shmget(int key, unsigned int size, int shmflag) {

  acquire(&GLOBAL_BOOK.lock);
  if(key <= 0) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
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

    release(&GLOBAL_BOOK.lock);
    return lookup;
  }
  return -1;
}

void*
shmat(int shmid, const void *shmaddr, int shmflg) {

  int perm = 0, lookup = 0, noofpages = GLOBAL_BOOK.shmid_ds[shmid].no_of_pages;
  void* vir;
  if(shmid < 0 || shmid > MAX_REGIONS) {
    return (void*)-1;
  }
  if(shmaddr != 0) {
    if( (int)shmaddr > (int)KERNBASE || (int)shmaddr < (int)HEAPLIMIT ) {
      return (void*) -1;
    }
  }

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
      return (void*)-1;
    }
  }

  if(shmflg == 0) {
    if (GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_RW)
      perm = PTE_W | PTE_U;
    else {
      release(&GLOBAL_BOOK.lock);
      return (void*)-1;
    }
  }else if (shmflg == SHM_RDONLY) {

    if (GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_R)
      perm = PTE_U;
    else {
      release(&GLOBAL_BOOK.lock);
      return (void*)-1;
    }
  }
  if(shmaddr == (void*)0)
    vir = curproc->sharedmem.next_virtual;
  else
    vir = (void*)shmaddr;
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
  GLOBAL_BOOK.shmid_ds[shmid].shm_lpid = curproc->pid;

  for(int i = 0; i < noofpages; i++) {
    if (mappages(curproc->pgdir, vir, PGSIZE, (uint)GLOBAL_BOOK.shmid_ds[shmid].v2p[i], perm) < 0) {
      deallocuvm(curproc->pgdir, (uint)vir, (uint) vir + GLOBAL_BOOK.shmid_ds[shmid].shm_segsz);
      release(&GLOBAL_BOOK.lock);
      return (void*)-1;
    }
    vir = vir + PGSIZE;
  }
  curproc->sharedmem.next_virtual = vir;
  release(&GLOBAL_BOOK.lock);
  return curproc->sharedmem.sharedseg[lookup].viraddr;
}

int
shmdt(const void *shmaddr) {

  struct proc *curproc = myproc();
  int shmid = -1, lookup = -1;
  void *vir, *vir2, *phy;
  pde_t *pte;

  if(curproc->sharedmem.noofshmreg <= 0) {
    return -1;
  }
  for(int i = 0; i < MAX_REGIONS_PER_PROC; i++) {
    if((shmaddr == curproc->sharedmem.sharedseg[i].viraddr) && (curproc->sharedmem.sharedseg[i].key != -1)) {
      shmid = curproc->sharedmem.sharedseg[i].shmid;
      lookup = i;
      break;
    }
  }
  if(shmid == -1) {
    return -1;
  }
  vir = curproc->sharedmem.sharedseg[lookup].viraddr;
  vir2 = vir;
  if(vir == 0) {
    return -1;
  }
  acquire(&GLOBAL_BOOK.lock);
  GLOBAL_BOOK.shmid_ds[shmid].shm_nattch--;

  for (int i = 0; i < GLOBAL_BOOK.shmid_ds[shmid].no_of_pages; i++) {

    if((pte = walkpgdir(curproc->pgdir, vir2, 0)) == 0) {
      release(&GLOBAL_BOOK.lock);
      return -1;
    }
    *pte = 0;
    vir2 = vir2 + PGSIZE;
  }
  curproc->sharedmem.next_virtual = (void*)((int)curproc->sharedmem.next_virtual - PGSIZE * curproc->sharedmem.sharedseg[lookup].noofpages);
  curproc->sharedmem.noofshmreg--;
  curproc->sharedmem.sharedseg[lookup].key = -1;
  curproc->sharedmem.sharedseg[lookup].noofpages = 0;
  curproc->sharedmem.sharedseg[lookup].perm = -1;
  curproc->sharedmem.sharedseg[lookup].shmid = -1;
  curproc->sharedmem.sharedseg[lookup].viraddr = (void*)0;
  GLOBAL_BOOK.shmid_ds[shmid].shm_lpid = curproc->pid;

  if(GLOBAL_BOOK.shmid_ds[shmid].shm_nattch <= 0 && GLOBAL_BOOK.shmid_ds[shmid].isdelete == 1) {

    for (int i = 0; i < GLOBAL_BOOK.shmid_ds[shmid].no_of_pages; i++) {
      phy = GLOBAL_BOOK.shmid_ds[shmid].v2p[i];
      GLOBAL_BOOK.shmid_ds[shmid].v2p[i] = (void*)0;
      kfree((char*)P2V(phy));
    }

    GLOBAL_BOOK.shmid_ds[shmid].no_of_pages = 0;
    GLOBAL_BOOK.shmid_ds[shmid].shm_cpid = -1;
    GLOBAL_BOOK.shmid_ds[shmid].shm_lpid = curproc->pid;
    GLOBAL_BOOK.shmid_ds[shmid].shm_nattch = 0;
    GLOBAL_BOOK.shmid_ds[shmid].shm_segsz = 0;
    GLOBAL_BOOK.shmid_ds[shmid].shmid = -1;
    GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode = -1;
    GLOBAL_BOOK.shmid_ds[shmid].shm_perm.__key = -1;
  }
  release(&GLOBAL_BOOK.lock);
  return 0;
}

int
shmctl(int shmid, int cmd, struct shmid_ds *buf) {

  acquire(&GLOBAL_BOOK.lock);
  if(shmid < 0 || shmid > MAX_REGIONS) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  if(GLOBAL_BOOK.shmid_ds[shmid].shm_perm.__key == -1) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  if(buf == 0) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  if (cmd == IPC_STAT) {
    if(GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_R || GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode == SHM_RW) {
      buf->no_of_pages = GLOBAL_BOOK.shmid_ds[shmid].no_of_pages;
      buf->shm_cpid = GLOBAL_BOOK.shmid_ds[shmid].shm_cpid;
      buf->shm_lpid = GLOBAL_BOOK.shmid_ds[shmid].shm_lpid;
      buf->shm_nattch = GLOBAL_BOOK.shmid_ds[shmid].shm_nattch;
      buf->shm_segsz = GLOBAL_BOOK.shmid_ds[shmid].shm_segsz;
      buf->shmid = GLOBAL_BOOK.shmid_ds[shmid].shmid;
      buf->shm_perm.mode = GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode;
      buf->shm_perm.__key = GLOBAL_BOOK.shmid_ds[shmid].shm_perm.__key;
      for (int i = 0; i < MAX_PAGES; i++) {
        buf->v2p[i] = GLOBAL_BOOK.shmid_ds[shmid].v2p[i];
      }
    }
  } else if (cmd == IPC_INFO) {
    buf->shminfo.shmall = GLOBAL_BOOK.shmid_ds[shmid].shminfo.shmall;
    buf->shminfo.shmmax = GLOBAL_BOOK.shmid_ds[shmid].shminfo.shmmax;
    buf->shminfo.shmmin = GLOBAL_BOOK.shmid_ds[shmid].shminfo.shmmin;
    buf->shminfo.shmmni = GLOBAL_BOOK.shmid_ds[shmid].shminfo.shmmni;
    buf->shminfo.shmseg = GLOBAL_BOOK.shmid_ds[shmid].shminfo.shmseg;

  } else if (cmd == IPC_SET) {
    if(buf->shm_perm.mode == SHM_R || buf->shm_perm.mode == SHM_RW) {
      GLOBAL_BOOK.shmid_ds[shmid].shm_perm.mode = buf->shm_perm.mode;
    }
  } else if (cmd == IPC_RMID) {

    GLOBAL_BOOK.shmid_ds[shmid].isdelete = 1;
  } else {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  release(&GLOBAL_BOOK.lock);
  return 0;
}
