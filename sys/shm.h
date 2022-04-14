#define MAX_PAGES   32
#define MAX_REGION  128



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

