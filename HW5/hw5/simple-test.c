#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LibFS.h"

void usage(char *prog)
{
  printf("USAGE: %s <disk_image_file>\n", prog);
  exit(1);
}

int main(int argc, char *argv[])
{
  if (argc != 2) usage(argv[0]);

  if(FS_Boot(argv[1]) < 0) {
    printf("ERROR: can't boot file system from file '%s'\n", argv[1]);
    return -1;
  } else printf("file system booted from file '%s'\n", argv[1]);

  char* fn;

  fn = "/first-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);

  fn = "/second-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);

  fn = "/first-dir";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' created successfully\n", fn);
  
  fn = "/first-dir/second-dir";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' created successfully\n", fn);

  fn = "/first-dir/second-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);

  fn = "/first-dir/second-dir/first-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);
  
  /*fn = "/first-file/second-dir";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' created successfully\n", fn);

  fn = "/first_dir/third*dir";
  if(Dir_Create(fn) < 0) printf("ERROR: can't create dir '%s'\n", fn);
  else printf("dir '%s' created successfully\n", fn);
  
  fn = "/first-file";
  if(File_Unlink(fn) < 0) printf("ERROR: can't unlink file '%s'\n", fn);
  else printf("file '%s' unlinked successfully\n", fn);


   fn = "/first-file";
  if(File_Create(fn) < 0) printf("ERROR: can't create file '%s'\n", fn);
  else printf("file '%s' created successfully\n", fn);

  fn = "/first-dir";
  if(Dir_Unlink(fn) < 0) printf("ERROR: can't unlink dir '%s'\n", fn);
  else printf("dir '%s' unlinked successfully\n", fn);

  fn = "/first-dir/second-dir";
  if(Dir_Unlink(fn) < 0) printf("ERROR: can't unlink dir '%s'\n", fn);
  else printf("dir '%s' unlinked successfully\n", fn);

  */fn = "/first-dir/second-file";
  int fd = File_Open(fn);
  if(fd < 0) printf("ERROR: can't open file '%s'\n", fn);
  else printf("file '%s' opened successfully, fd=%d\n", fn, fd);

 char buf[1280]; char* ptr = buf;
  int i;
  for(i=0; i<10000; i++) {
    sprintf(ptr, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr += strlen(ptr);
    if(ptr >= buf+1000) break;
  }

  //printf("This is the buffer before writing to disk:\n%s\n", buf);
  //printf("bu[0] = %c\nbu[1] = %c\nbu[2]= %c\nbu[1022] = %c\nbu[1023] = %c\n", buf[0], buf[1], buf[2], buf[1022], buf[1023]);

  if(File_Write(fd, buf, 1280) != 1280)
    printf("ERROR: can't write 1024 bytes to fd=%d\n", fd);
  else printf("successfully wrote 1024 bytes to fd=%d\n", fd);


  if(File_Close(fd) < 0) printf("ERROR: can't close fd %d\n", fd);
  else printf("fd %d closed successfully\n", fd);

  /////////////////////////////////////////////

  fn = "/first-dir/second-dir/first-file";
  fd = File_Open(fn);
  if(fd < 0) printf("ERROR: can't open file '%s'\n", fn);
  else printf("file '%s' opened successfully, fd=%d\n", fn, fd);

  ptr = buf;
  
  for(i=0; i<1000; i++) {
    sprintf(ptr, "%d %s", i, (i+1)%10==0 ? "\n" : "");
    ptr += strlen(ptr);
    if(ptr >= buf+1000) break;
  }

  //printf("This is the buffer before writing to disk:\n%s\n", buf);
  //printf("bu[0] = %c\nbu[1] = %c\nbu[2]= %c\nbu[1022] = %c\nbu[1023] = %c\n", buf[0], buf[1], buf[2], buf[1022], buf[1023]);

  if(File_Write(fd, buf, 1024) != 1024)
    printf("ERROR: can't write 1024 bytes to fd=%d\n", fd);
  else printf("successfully wrote 1024 bytes to fd=%d\n", fd);


  if(File_Close(fd) < 0) printf("ERROR: can't close fd %d\n", fd);
  else printf("fd %d closed successfully\n", fd);


  fn = "/first-dir";
  int a;
  a = Dir_Size(fn);
  printf("This is the size of file %s is = %d\n", fn, a);
  ////////////////


  //open the same file after writing on it
  /*fn = "/second-file";
  fd = File_Open(fn);
  if(fd < 0) printf("ERROR: can't open file '%s'\n", fn);
  else printf("file '%s' opened successfully, fd=%d\n", fn, fd);

  char bufread[1024];

  if(File_Seek(fd, 0) == -1){
    printf("Error in File Seek\n");
  }else{
      if(File_Read(fd, bufread, 1024) < 0)
        printf("ERROR: can't read 1024 bytes to fd=%d\n", fd);
      else printf("successfully Read 1024 bytes to fd=%d\n", fd);  }

  
  if(File_Close(fd) < 0) printf("ERROR: can't close fd %d\n", fd);
  else printf("fd %d closed successfully\n", fd);*/
  
  if(FS_Sync() < 0) {
    printf("ERROR: can't sync file system to file '%s'\n", argv[1]);
    return -1;
  } else printf("file system sync'd to file '%s'\n", argv[1]);
    
  return 0;
}
