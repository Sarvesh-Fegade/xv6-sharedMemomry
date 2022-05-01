#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "sys/shm.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  void *a;
  int id;
  // id = shmget(123, 5024, IPC_CREAT | IPC_EXCL | 0400);
  // printf(2, "id is = %d\n", id);
  // a = shmat(id, 0, SHM_RDONLY);
  // printf(2, "address is = %x\n", a);
  // id = shmget(1234, 8192, IPC_CREAT | IPC_EXCL | 0600);
  // printf(2, "id is = %d\n", id);
  // //shmget(1256, 1024, IPC_CREAT | IPC_EXCL | 0600);
  // a = shmat(id, 0, 0);
  // printf(2, "address is = %x\n", a);

  id = shmget(6448, 1024, IPC_CREAT | IPC_EXCL | 0400);
  printf(2, "id is = %d\n", id);
  a = shmat(id, 0, SHM_RDONLY);
  //a = shmat(id, 0, SHM_RDONLY);
  printf(2, "address is = %x\n", a);

  id = shmdt((void *)a);
  printf(2, "id is = %d\n", id);
  a = shmat(id, 0, SHM_RDONLY);
  //a = shmat(id, 0, SHM_RDONLY);
  printf(2, "address is = %x\n", a);


  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}
