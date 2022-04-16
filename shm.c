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

void
initsharedmemory(void) {

  initlock(&GLOBAL_BOOK.lock, "SHM");
  acquire(&GLOBAL_BOOK.lock);

  for (int i = 0; i < MAX_REGIONS; i++) {
    GLOBAL_BOOK.shmid_ds[i].shm_perm.__key = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_perm.mode = 0;
    GLOBAL_BOOK.shmid_ds[i].shminfo.shmmax = MAX_PAGES*PGSIZE;
    GLOBAL_BOOK.shmid_ds[i].shminfo.shmmin = 1;
    GLOBAL_BOOK.shmid_ds[i].shminfo.shmmni = MAX_REGIONS;
    GLOBAL_BOOK.shmid_ds[i].shminfo.shmseg = MAX_REGIONS_PER_PROC;
    GLOBAL_BOOK.shmid_ds[i].shminfo.shmall = 64000;
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
shmget(unsigned int key, unsigned int size, int shmflag) {

  acquire(&GLOBAL_BOOK.lock);

  int noofpages;
  if(size <= 0) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  noofpages = size / PGSIZE;
  noofpages += 1;
  if(noofpages > MAX_PAGES) {
    release(&GLOBAL_BOOK.lock);
    return -1;
  }
  

  

  return 0;






}








