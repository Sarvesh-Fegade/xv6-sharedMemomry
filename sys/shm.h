#define MAX_PAGES   32
#define MAX_REGION  128

#define SHM_R		0400	/* or S_IRUGO from <linux/stat.h> */
#define SHM_W		0200	/* or S_IWUGO from <linux/stat.h> */
#define SHM_RW      0600

#define	SHM_RDONLY	010000	/* read-only access */
#define	SHM_RND		020000	/* round attach address to SHMLBA boundary */
#define	SHM_REMAP	040000	/* take-over region on attach */
#define	SHM_EXEC	0100000 /* execution access */



struct shmid_ds {
    // struct ipc_perm shm_perm;
    unsigned int __key;
    unsigned int mode;
    int shmid;
    int shm_segsz;
    int shm_cpid;
    int shm_lpid;
    int shm_nattch;
    int no_of_pages;
    void* v2p[MAX_PAGES];    
};

