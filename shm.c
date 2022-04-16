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
  struct shmid_ds shmid_ds[MAX_REGION];
}GLOBAL_BOOK;


void
initsharedmemory(void) {
  initlock(&GLOBAL_BOOK.lock, "SHM");
  acquire(&GLOBAL_BOOK.lock);

  for (int i = 0; i < MAX_REGION; i++) {
    GLOBAL_BOOK.shmid_ds[i].shm_nattch = 0;
    GLOBAL_BOOK.shmid_ds[i].shm_cpid = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_lpid = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_perm.__key = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_segsz = -1;
    GLOBAL_BOOK.shmid_ds[i].shm_perm.mode = 0;
    GLOBAL_BOOK.shmid_ds[i].no_of_pages = 0;
    GLOBAL_BOOK.shmid_ds[i].shmid = i;
    for(int j = 0; j < MAX_PAGES; j++) {
      GLOBAL_BOOK.shmid_ds[i].v2p[j] = (void*)0;
    }
  }
  release(&GLOBAL_BOOK.lock);
}

int
shmget(unsigned int key, unsigned int size, int shmflag) {

  return 0;






}








