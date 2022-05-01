#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "sys/shm.h"

int main() {
    
    int key1 = 3245, id1, dt, cmp;
    void *addr, *addr2;
    char *ptr;

    id1 = shmget(key1, 1024, IPC_CREAT | 0600);
    if(id1 < 0)
        printf(1, "shmget --> Failed with %d\n", id1);
    else
        printf(1, "shmget --> Passed with %d\n", id1);

    addr = shmat(id1, 0, 0);
    printf(1, "Address returned by shmat = %x\n", addr);

    if(addr < 0)
        printf(1, "shmat --> Failed \n");
    else
        printf(1, "shmat --> Passed \n");

    ptr = (char*)addr;
    ptr = "abcd";
    dt = shmdt(addr);
    if(dt == 0 )
        printf(1, "shmdt --> Passed \n");
    else
        printf(1, "shmdt --> Failed \n");

    addr2 = shmat(id1, 0, 0);
    printf(1, "Address returned by shmat = %x\n", addr2);
    if(addr2 < 0)
        printf(1, "shmat --> Failed \n");
    else
        printf(1, "shmat --> Passed \n");

    if(addr == addr2)
        printf(1, "Both Address same --> Passed \n");
    else
        printf(1, "Addresses not equal --> Failed \n");

    cmp = strcmp(ptr, "abcd");
    if(cmp == 0)
        printf(1, "Write test --> Passed  %d \n", cmp);
    else
        printf(1, "Write test --> Failed   %d \n", cmp);

    exit();
}