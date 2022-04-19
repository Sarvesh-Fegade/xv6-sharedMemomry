#define MAX_PAGES   16   // maz pages per shm region
#define MAX_REGIONS  3840  // total regions ,max 
#define MAX_REGIONS_PER_PROC 8

#define SHM_R		0400	/* or S_IRUGO from <linux/stat.h> */
#define SHM_W		0200	/* or S_IWUGO from <linux/stat.h> */
#define SHM_RW      0600

#define	SHM_RDONLY	010000	/* read-only access */
#define	SHM_RND		020000	/* round attach address to SHMLBA boundary */
#define	SHM_REMAP	040000	/* take-over region on attach */
#define	SHM_EXEC	0100000 /* execution access */

#define IPC_CREAT  00001000   /* create if key is nonexistent */
#define IPC_EXCL   00002000   /* fail if key exists */
#define IPC_NOWAIT 00004000   /* return error on wait */
#define IPC_PRIVATE 0

struct	shminfo {
	int shmmax;     /* Maximum segment size */
	int shmmin;     /* Minimum segment size; always 1 */
	int shmmni;     /* Maximum number of segments */   
	int shmseg;     /* Maximum number of segments that a process can attach; unused within kernel */
	int shmall;     /* Maximum number of pages of shared memory, system-wide */
};

struct ipc_perm {
    unsigned int __key;
    unsigned int mode;
};

struct shmid_ds {
    struct ipc_perm shm_perm;
    //struct shminfo shminfo;
    // unsigned int __key;
    // unsigned int mode;
    int shmid;
    int shm_segsz;
    int shm_cpid;
    int shm_lpid;
    int shm_nattch;
    int no_of_pages;
    void* v2p[MAX_PAGES];
};

