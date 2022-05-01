#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "sys/shm.h"

int main() {

    int id, id1, id2, id3, dt, ctl;
    void *at, *at1;
    struct shmid_ds buf, buf2;
    buf2.shm_perm.mode = SHM_R;
    // Test Cases for shmget
    id = shmget(4325, 1024, IPC_CREAT | 0600);
    id3 = id;
    if(id < 0)
        printf(1, "Test: new segment with IPC_CREAT with RW -> Fail\n");
    else
        printf(1, "Test: new segment with IPC_CREAT with RW -> Pass\n");

    id1 = shmget(3514, 2034, IPC_CREAT | 0400);
    if(id1 < 0)
        printf(1, "Test: new segment with IPC_CREAT with only R -> Fail\n");
    else
        printf(1, "Test: new segment with IPC_CREAT with Only R -> Pass\n");
    id2 = shmget(7652, 7034, IPC_CREAT | IPC_EXCL | 0600);
    if(id2 < 0)
        printf(1, "Test: new segment with IPC_CREAT with EXCL with RW -> Fail\n");
    else
        printf(1, "Test: new segment with IPC_CREAT with RW -> Pass\n");

    id = shmget(4524, 3024, IPC_CREAT | IPC_EXCL | 0400);
    if(id < 0)
        printf(1, "Test: new segment with IPC_CREAT with EXCL with only R -> Fail\n");
    else
        printf(1, "Test: new segment with IPC_CREAT with only R -> Pass\n");

    id = shmget(4524, 3024, IPC_CREAT | IPC_EXCL | 0400);
    if(id < 0)
        printf(1, "Test: Requesting alread existing region with IPC_EXCL -> Pass\n");
    else
        printf(1, "Test: Requesting alread existing region with IPC_EXCL -> Fail\n");

    id = shmget(4325, 0, IPC_CREAT | 0600);
    if(id < 0)
        printf(1, "Test: Segment with size 0 -> Pass\n");
    else
        printf(1, "Test: Segment with size 0 -> Fail\n");

    id = shmget(4325, 999999999, IPC_CREAT | 0600);
    if(id < 0)
        printf(1, "Test: Segment with overflow -> Pass\n");
    else
        printf(1, "Test: Segment with overflow -> Fail\n");

    id = shmget(-131, 1024, IPC_CREAT | 0600);
    if(id < 0)
        printf(1, "Test: shmget with invalid key -> Pass\n");
    else
        printf(1, "Test: shmget with invalid key -> Fail  %d\n", id);

    id = shmget(4245, 0, IPC_CREAT | 0300 );
    if(id < 0)
        printf(1, "Test: Invalid permission argument -> Pass\n");
    else
        printf(1, "Test: Invalid permission argument -> Fail\n");

    //Test Cases for shmat

    at = shmat(id3, 0, 0);
    if((int)at)
        printf(1, "Test: shmat with default permission and NULL address -> Pass \n");
    else
        printf(1, "Test: shmat with default permission and NULL address -> Fail \n");

    at1 = shmat(id1, 0, SHM_RDONLY);
    if((int)at1)
        printf(1, "Test: shmat with RDONLY and NULL address -> Pass \n");
    else
        printf(1, "Test: shmat with RDONLY and NULL address -> Fail \n");

    at1 = shmat(id1, 0, 0);
    if((int)at1 < 0)
        printf(1, "Test: attaching RDONLY segment with RW pwrmission -> Pass  \n");
    else
        printf(1, "Test: attaching RDONLY segment with RW pwrmission -> Fail \n");

    at1 = shmat(512, 0, 0);
    if((int)at1 < 0)
        printf(1, "Test: shmat with invalid shmid -> Pass \n");
    else
        printf(1, "Test: shmat with invalid shmid -> Fail \n");

    at1 = shmat(4366, 0, 0);
    if((int)at1 < 0)
        printf(1, "Test: shmat with invalid shmid -> Pass \n");
    else
        printf(1, "Test: shmat with invalid shmid -> Fail \n");

    at1 = shmat(100, 0, 0);
    if((int)at1 < 0)
        printf(1, "Test: shmat with unsigned shmid -> Pass \n");
    else
        printf(1, "Test: shmat with unsigned shmid -> Fail \n");

    at1 = shmat(id1, (void*)0x70040000, SHM_RDONLY);
    if((int)at1)
        printf(1, "Test: shmat with user-specified address -> Pass \n");
    else
        printf(1, "Test: shmat with user-specified address -> Fail \n");

    at1 = shmat(id1, (void*)0x60000000, SHM_RDONLY);
    if((int)at1 < 0)
        printf(1, "Test: shmat with address lower than heaplimit -> Pass \n");
    else
        printf(1, "Test: shmat with address lower than heaplimit -> Fail \n");

    at1 = shmat(id2, (void*)0x80005000, SHM_RDONLY);
    if((int)at1 < 0)
        printf(1, "Test: shmat with address higher than kernbase -> Pass \n");
    else
        printf(1, "Test: shmat with address higher than kernbase -> Fail \n");

    at1 = shmat(id1, (void*)0x70040000, SHM_RDONLY);
    if((int)at1)
        printf(1, "Test: shmat with alread attached region -> Pass \n");
    else
        printf(1, "Test: shmat with alread attached region -> Fail \n");

    // Test Cases for shmdt
    dt = shmdt(at);
    if(dt == 0)
        printf(1, "Test: dettaching attached addredd -> Pass \n");
    else
        printf(1, "Test: dettaching attached addredd -> Fail \n");

    dt = shmdt((void*)0x70050000);
    if(dt == 0)
        printf(1, "Test: dettaching unattached addredd -> Fail \n");
    else
        printf(1, "Test: dettaching unattached addredd -> Pass \n");

    // Test Cases for shmctl
    ctl = shmctl(id3, IPC_STAT, &buf);
    if(ctl == 0)
        printf(1, "Test: shmctl with IPC_STAT -> Pass \n");
    else
        printf(1, "Test: shmctl with IPC_STAT -> Fail \n");
    ctl = shmctl(id3, IPC_INFO, &buf);
    if(ctl == 0)
        printf(1, "Test: shmctl with IPC_INFO -> Pass \n");
    else
        printf(1, "Test: shmctl with IPC_INFO -> Fail \n");
    ctl = shmctl(id3, IPC_SET, &buf2);
    if(ctl == 0)
        printf(1, "Test: shmctl with IPC_SET -> Pass \n");
    else
        printf(1, "Test: shmctl with IPC_SET -> Fail \n");
    
    ctl = shmctl(id3, IPC_RMID, &buf2);
    if(ctl == 0)
        printf(1, "Test: shmctl with IPC_RMID -> Pass \n");
    else
        printf(1, "Test: shmctl with IPC_RMID -> Fail \n");
    exit();
}