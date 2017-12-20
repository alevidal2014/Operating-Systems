#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "LibDisk.h"
#include "LibFS.h"
#include <ctype.h>

// set to 1 to have detailed debug print-outs and 0 to have none
#define FSDEBUG 1

#if FSDEBUG
#define dprintf printf
#else
#define dprintf noprintf
void noprintf(char* str, ...) {}
#endif

// the file system partitions the disk into five parts:

// 1. the superblock (one sector), which contains a magic number at
// its first four bytes (integer)
#define SUPERBLOCK_START_SECTOR 0

// the magic number chosen for our file system
#define OS_MAGIC 0xdeadbeef

// 2. the inode bitmap (one or more sectors), which indicates whether
// the particular entry in the inode table (#4) is currently in use
#define INODE_BITMAP_START_SECTOR 1

// the total number of bytes and sectors needed for the inode bitmap;
// we use one bit for each inode (whether it's a file or directory) to
// indicate whether the particular inode in the inode table is in use
#define INODE_BITMAP_SIZE ((MAX_FILES+7)/8)                                           //125.875 => 125 in our program
#define INODE_BITMAP_SECTORS ((INODE_BITMAP_SIZE+SECTOR_SIZE-1)/SECTOR_SIZE)          //1.24 => 1 in our program

// 3. the sector bitmap (one or more sectors), which indicates whether
// the particular sector in the disk is currently in use
#define SECTOR_BITMAP_START_SECTOR (INODE_BITMAP_START_SECTOR+INODE_BITMAP_SECTORS)   // 2 in our program

// the total number of bytes and sectors needed for the data block
// bitmap (we call it the sector bitmap); we use one bit for each
// sector of the disk to indicate whether the sector is in use or not
#define SECTOR_BITMAP_SIZE ((TOTAL_SECTORS+7)/8)                                      //1250.87 => 1250 in our program
#define SECTOR_BITMAP_SECTORS ((SECTOR_BITMAP_SIZE+SECTOR_SIZE-1)/SECTOR_SIZE)        //3.44 => 3 in our program

// 4. the inode table (one or more sectors), which contains the inodes
// stored consecutively
#define INODE_TABLE_START_SECTOR (SECTOR_BITMAP_START_SECTOR+SECTOR_BITMAP_SECTORS)   // 5 in our program

// an inode is used to represent each file or directory; the data
// structure supposedly contains all necessary information about the
// corresponding file or directory
typedef struct _inode {
  int size; // the size of the file or number of directory entries
  int type; // 0 means regular file; 1 means directory
  int data[MAX_SECTORS_PER_FILE]; // indices to sectors containing data blocks
} inode_t;

// the inode structures are stored consecutively and yet they don't
// straddle accross the sector boundaries; that is, there may be
// fragmentation towards the end of each sector used by the inode
// table; each entry of the inode table is an inode structure; there
// are as many entries in the table as the number of files allowed in
// the system; the inode bitmap (#2) indicates whether the entries are
// current in use or not
#define INODES_PER_SECTOR (SECTOR_SIZE/sizeof(inode_t))                             //4 in our program
#define INODE_TABLE_SECTORS ((MAX_FILES+INODES_PER_SECTOR-1)/INODES_PER_SECTOR)     //250.75 => 250 in our program

// 5. the data blocks; all the rest sectors are reserved for data
// blocks for the content of files and directories
#define DATABLOCK_START_SECTOR (INODE_TABLE_START_SECTOR+INODE_TABLE_SECTORS)       //255 in our program

// other file related definitions

// max length of a path is 256 bytes (including the ending null)
#define MAX_PATH 256

// max length of a filename is 16 bytes (including the ending null)
#define MAX_NAME 16

// max number of open files is 256
#define MAX_OPEN_FILES 256

// each directory entry represents a file/directory in the parent
// directory, and consists of a file/directory name (less than 16
// bytes) and an integer inode number
typedef struct _dirent {
  char fname[MAX_NAME]; // name of the file
  int inode; // inode of the file
} dirent_t;

// the number of directory entries that can be contained in a sector
#define DIRENTS_PER_SECTOR (SECTOR_SIZE/sizeof(dirent_t))               //25 in our program

// global errno value here
int osErrno;

// the name of the disk backstore file (with which the file system is booted)
static char bs_filename[1024];

/* the following functions are internal helper functions */

// check magic number in the superblock; return 1 if OK, and 0 if not
static int check_magic()
{
  char buf[SECTOR_SIZE];
  if(Disk_Read(SUPERBLOCK_START_SECTOR, buf) < 0)
    return 0;
  if(*(int*)buf == OS_MAGIC) return 1;
  else return 0;
}

// initialize a bitmap with 'num' sectors starting from 'start'
// sector; all bits should be set to zero except that the first
// 'nbits' number of bits are set to one
static void bitmap_init(int start, int num, int nbits)
{
  /*********BEGING OUR CODE **********/
  //printf("***************Inside bitmap init*************\n");
  int nbytes = nbits/8;                       // Number of bytes to set to 1  
  int extra_bits = nbits % 8;                 //Remainder bits after the last byte set to 1
  int final_block = 0;                        //Bool varaible to know which is the last block to set bits to 1
  char buf[SECTOR_SIZE];                      //Buffer sector  
  //printf("# of bytes to set to 1 = %d\n", nbytes);
  //printf("# of extra bits to set to 1 = %d\n", extra_bits);

  int i;
  for(i=start; i<(start+num); i++){     //  Going through the sectors to initialize all its bytes to 
    
    if(nbytes < SECTOR_SIZE){
      final_block = 1;            //Still more blocks 
    }

    if(Disk_Read(i, buf) < 0){                                      //Read the sector
      dprintf("... failed reading the block %d\n" , i);
      osErrno = E_GENERAL;
      return;  
    }
    
   
    memset(buf, 0, SECTOR_SIZE);     //Set all the bytes to 0

    if(nbytes >= 0){               //At least 1 byte to set to 1
        if(final_block == 0){             //If is more than 1 sector
          memset(buf, 1, SECTOR_SIZE);    //Set the whole sector to 1
          nbytes = nbytes - SECTOR_SIZE;  //Decrement the remaining bytes to set
        }else{
          memset(buf, 255, nbytes);         //Set to 1 only the bytes within the sector 

          if(extra_bits > 0 && final_block == 1){               //If there is remaining bits to set to 1 and this is the final block to set 1s
          unsigned char mask = (255<<(8 - extra_bits));    //Mask use to set the last byte 255 = 1111 1111 (we shift left as meny 0 we need in the last byte)
          //printf("Mask used = %d\n", mask);  
          buf[nbytes] = mask;                     //Set the last byte back to the buffer
         /* printf("This is buffer index 0 = %d\n", (unsigned char)buf[0]); 
          printf("This is buffer index 1 = %d\n", (unsigned char)buf[1]);   
          printf("This is buffer index 2 = %d\n", (unsigned char)buf[2]); 
          printf("last byte full = %d  at index %d\n", (unsigned char)buf[nbytes-1], nbytes-1);  
          printf("next byte to last full = %d at index %d\n", (unsigned char)buf[nbytes], nbytes);*/
          nbytes = -1;
          }
        }
    }      

    if(Disk_Write(i, buf) < 0) {                                     //Write the sector back
      dprintf("... failed writing the block %d\n" , i);
      osErrno = E_GENERAL;
      return;
    }
  }

/********* END OUR CODE **********/
}

/********* BEGING OUR CODE **********/
//Helper Method to find if a specific bit is set inside a byte
//Received the byte and the position inside the byte to check
//Return 1 if the bit is set in this position or 0 otherwise
static int isNthBitSet (unsigned char c, int n) {
    static unsigned char mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    return ((c & mask[n]) != 0);
}

//Helper Method to Set if a specific bit inside a byte
//Received the byte and the position inside the byte to set
//Return the byte after the bit was set at position n
static char setNthBitSet (unsigned char c, int n) {
    static unsigned char mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    return (c | mask[n]);
}
/********* END OUR CODE **********/

// set the first unused bit from a bitmap of 'nbits' bits (flip the
// first zero appeared in the bitmap to one) and return its location;
// return -1 if the bitmap is already full (no more zeros)
static int bitmap_first_unused(int start, int num, int nbits)
{
  /********* BEGING OUR CODE **********/
  //printf("***********Inside bitmap_first_unused************\n");
  //printf("nbits received = %d", nbits);

  int nbytes = nbits/8;                       // Number of bytes to set to 1  
  int extra_bits = nbits % 8;                 //Remainder bits after the last byte set to 1
  int final_block = 0;                        //Bool varaible to know if I am in the last byte
  char buf[SECTOR_SIZE];                      //Buffer sector  
  int location = -1;                          //Return location 
  
  int i;
  for(i=start; i<(start+num); i++){     //  Going through the sectors to check all its bytes 

    if(nbytes < SECTOR_SIZE){
      final_block = 1;            //This is the last sector
    }

    if(Disk_Read(i, buf) < 0){                                      //Read the sector
      dprintf("... failed reading the block %d\n" , i);
      osErrno = E_GENERAL;
      return -1;  
    }

    //Variables used to control the following loops
    int b;       
    int bitInbyte; 
    int limit; 

    if(final_block == 1){                  //This is the last sector
      limit = nbytes;
    }else{                                  //We need to check more than one sector
      limit = SECTOR_SIZE;
    } 
     
      //dprintf("---------Inside my set first unused method Limit = %d\n" , limit);                              
    for(b=0; b<limit; b++){                          //Going through all the completed bytes in the sector
      for(bitInbyte =0; bitInbyte<8; bitInbyte++){  //Checking all the bits inside this byte
        location ++;
        //dprintf("---------Inside my set first unused method location = %d\n" , location);
       // dprintf("byte sent to check %d and position %d and the result is %d\n", (unsigned char)buf[b], location, isNthBitSet(buf[b], bitInbyte));
        if(!isNthBitSet(buf[b], bitInbyte)){        //Chek if this bit is not set          
           
           buf[b] = setNthBitSet(buf[b], bitInbyte);          //Set this bit
           //dprintf("buffer value after bit is set %d\n", (unsigned char)buf[b]);
          if(Disk_Write(i, buf) < 0) {              //Write the sector back
            dprintf("... failed writing the block %d\n" , i);
            osErrno = E_GENERAL;
            return -1;
          }

          return location;        //Return the location found 
        }
        
      }   //End of the loop through the bits within the byte   
    }   //End of the loop through the bytes within the sector

    //If we reach this point we still need to do a last check
    //1- We need to check the last bits in the last incompleted byte
    if(final_block == 1){   //We need to check the last byte
      for(bitInbyte =0; bitInbyte < extra_bits ; bitInbyte++){  //Checking the remaining bits in this byte
        location ++;
        //dprintf("---------Inside my set first unused method location = %d\n" , location);
        if(!isNthBitSet(buf[b], bitInbyte)){        //Chek if this bit is not set
           buf[b] = setNthBitSet(buf[b], bitInbyte);          //Set this bit

          if(Disk_Write(i, buf) < 0) {              //Write the sector back
            dprintf("... failed writing the block %d\n" , i);
            osErrno = E_GENERAL;
            return -1;
          }

          return location;        //Return the location found 
        }
      }   //End of the loop inside the last byte
    }

  } // End of the loop of the sector  
  /********* END OUR CODE **********/
  return -1;
}

// reset the i-th bit of a bitmap with 'num' sectors starting from
// 'start' sector; return 0 if successful, -1 otherwise
static int bitmap_reset(int start, int num, int ibit)
{
  /*********BEGING OUR CODE **********/
  //printf("***************Inside bitmap init*************\n");
  int nbytes = ibit/8;                       // Number of completed bytes before the one that contains the bit we are going to reset  
  int extra_bits = ibit % 8;                 //Position of the bit to reset within the last byte
  char buf[SECTOR_SIZE];                      //Buffer sector  
  //printf("# of bytes to set to 1 = %d\n", nbytes);
  //printf("# of extra bits to set to 1 = %d\n", extra_bits);

        
    if(nbytes > SECTOR_SIZE*num){
      dprintf("... Error ibit=%d passed to reset is to big for a sector \n" , ibit);  //incorrect number of ibit (greater than the sector size)
      return -1;                 
    }

    while(nbytes> SECTOR_SIZE){
      nbytes = nbytes- SECTOR_SIZE;   //Go straight to the last sector 
    }

    if(Disk_Read(start, buf) < 0){                                      //Read the sector
      dprintf("... failed reading the block %d\n" , start);
      osErrno = E_GENERAL;
      return -1;  
    }

    static unsigned char mask[] = {127, 191, 223, 239, 247, 251, 253, 254};
    buf[nbytes] = (buf[nbytes] & mask[extra_bits]);

    if(Disk_Write(start, buf) < 0) {                                        //Write the sector back
            dprintf("... failed writing the block %d\n" , start);
            osErrno = E_GENERAL;
            return -1;
          }
    
  return 0;
  /********* END OUR CODE **********/
}

// return 1 if the file name is illegal; otherwise, return 0; legal
// characters for a file name include letters (case sensitive),
// numbers, dots, dashes, and underscores; and a legal file name
// should not be more than MAX_NAME-1 in length
static int illegal_filename(char* name)
{
  /********* BEGING OUR CODE **********/
  if(strlen(name)>MAX_NAME-1){                        //Cheking that the name is less than Max-Name-1
    printf("ERROR: Illegal file name (too long)\n");
    return 1;
  }else{
    int i;
    for(i=0; i< strlen(name); i++){         //Going through the letter in the name
      char check = name[i];
      if(!(isalpha(check) || isdigit(check) || check == '-' || check == '_'))     //Check for valid characteres
        return 1;
    }
    
  }
  return 0;
    /********* END OUR CODE **********/  
}

// return the child inode of the given file name 'fname' from the
// parent inode; the parent inode is currently stored in the segment
// of inode table in the cache (we cache only one disk sector for
// this); once found, both cached_inode_sector and cached_inode_buffer
// may be updated to point to the segment of inode table containing
// the child inode; the function returns -1 if no such file is found;
// it returns -2 is something else is wrong (such as parent is not
// directory, or there's read error, etc.)
static int find_child_inode(int parent_inode, char* fname, int *cached_inode_sector, char* cached_inode_buffer){

  int cached_start_entry = ((*cached_inode_sector)-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  int offset = parent_inode-cached_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* parent = (inode_t*)(cached_inode_buffer+offset*sizeof(inode_t));
  dprintf("... load parent inode: %d (size=%d, type=%d)\n",	parent_inode, parent->size, parent->type);
  if(parent->type != 1) {
    dprintf("... parent not a directory\n");
    return -2;
  }

  int nentries = parent->size; // remaining number of directory entries 
  int idx = 0;
  while(nentries > 0) {
    char buf[SECTOR_SIZE]; // cached content of directory entries
    if(Disk_Read(parent->data[idx], buf) < 0) return -2;
    int i;
    for(i=0; i<DIRENTS_PER_SECTOR; i++) {
      if(i>nentries) break;
      if(!strcmp(((dirent_t*)buf)[i].fname, fname)) {
	       // found the file/directory; update inode cache
	       int child_inode = ((dirent_t*)buf)[i].inode;
	       dprintf("... found child_inode=%d\n", child_inode);
	       int sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
	       if(sector != (*cached_inode_sector)) {
	         *cached_inode_sector = sector;
	         if(Disk_Read(sector, cached_inode_buffer) < 0) return -2;
	           dprintf("... load inode table for child\n");
	       }
	       return child_inode;
      }
    }
    idx++; nentries -= DIRENTS_PER_SECTOR;
  }
  dprintf("... could not find child inode\n");
  return -1; // not found
}

// follow the absolute path; if successful, return the inode of the
// parent directory immediately before the last file/directory in the
// path; for example, for '/a/b/c/d.txt', the parent is '/a/b/c' and
// the child is 'd.txt'; the child's inode is returned through the
// parameter 'last_inode' and its file name is returned through the
// parameter 'last_fname' (both are references); it's possible that
// the last file/directory is not in its parent directory, in which
// case, 'last_inode' points to -1; if the function returns -1, it
// means that we cannot follow the path
static int follow_path(char* path, int* last_inode, char* last_fname)
{
  if(!path) {
    dprintf("... invalid path\n");
    return -1;
  }
  if(path[0] != '/') {
    dprintf("... '%s' not absolute path\n", path);
    return -1;
  }
  
  // make a copy of the path (skip leading '/'); this is necessary
  // since the path is going to be modified by strsep()
  char pathstore[MAX_PATH]; 
  strncpy(pathstore, path+1, MAX_PATH-1);
  pathstore[MAX_PATH-1] = '\0'; // for safety
  char* lpath = pathstore;
  
  int parent_inode = -1, child_inode = 0; // start from root
  // cache the disk sector containing the root inode
  int cached_sector = INODE_TABLE_START_SECTOR;
  char cached_buffer[SECTOR_SIZE];
  if(Disk_Read(cached_sector, cached_buffer) < 0) return -1;
  dprintf("... load inode table for root from disk sector %d\n", cached_sector);
  
  // for each file/directory name separated by '/'
  char* token;
  while((token = strsep(&lpath, "/")) != NULL) {
    dprintf("... process token: '%s'\n", token);
    if(*token == '\0') continue; // multiple '/' ignored
    if(illegal_filename(token)) {
      dprintf("... illegal file name: '%s'\n", token);
      return -1; 
    }
    if(child_inode < 0) {
      // regardless whether child_inode was not found previously, or
      // there was issues related to the parent (say, not a
      // directory), or there was a read error, we abort
      dprintf("... parent inode can't be established\n");
      return -1;
    }
    parent_inode = child_inode;
    //printf("----------------Before calling find child inode: \n parent_inode = %d\t token = %s\t cached_sector = %d\t cached_buffer = %d\t\n", parent_inode, token, cached_sector, cached_buffer);
    child_inode = find_child_inode(parent_inode, token, &cached_sector, cached_buffer);
    //printf("----------------Return child inode= %d \n", child_inode);

    if(last_fname) strcpy(last_fname, token);
  }
  if(child_inode < -1) return -1; // if there was error, abort
  else {
    // there was no error, several possibilities:
    // 1) '/': parent = -1, child = 0
    // 2) '/valid-dirs.../last-valid-dir/not-found': parent=last-valid-dir, child=-1
    // 3) '/valid-dirs.../last-valid-dir/found: parent=last-valid-dir, child=found
    // in the first case, we set parent=child=0 as special case
    if(parent_inode==-1 && child_inode==0) parent_inode = 0;
    dprintf("... found parent_inode=%d, child_inode=%d\n", parent_inode, child_inode);
    *last_inode = child_inode;
    return parent_inode;
  }
}

// add a new file or directory (determined by 'type') of given name
// 'file' under parent directory represented by 'parent_inode'
int add_inode(int type, int parent_inode, char* file)
{
  // get a new inode for child
  //printf("******************Inside Add Inode*********************\n");
  int child_inode = bitmap_first_unused(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, INODE_BITMAP_SIZE);
  //printf("child Inode = %d\n", child_inode);

  if(child_inode < 0) {
    dprintf("... error: inode table is full\n");
    return -1; 
  }
  dprintf("... new child inode %d\n", child_inode);

  // load the disk sector containing the child inode
  int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
 // printf("Inode sector = %d\n", inode_sector);

  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);

  // get the child inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
 // printf("inode_start_entry = %d\n", inode_start_entry);

  int offset = child_inode-inode_start_entry;
  //printf("offset = %d\n", offset);

  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));

  // update the new child inode and write to disk
  memset(child, 0, sizeof(inode_t));
  child->type = type;
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... update child inode %d (size=%d, type=%d), update disk sector %d\n", child_inode, child->size, child->type, inode_sector);

  // get the disk sector containing the parent inode
  inode_sector = INODE_TABLE_START_SECTOR+parent_inode/INODES_PER_SECTOR;
  //printf(" Parent Inode sector = %d\n", inode_sector);
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for parent inode %d from disk sector %d\n", parent_inode, inode_sector);

  // get the parent inode
  inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  offset = parent_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* parent = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  dprintf("... get parent inode %d (size=%d, type=%d)\n", parent_inode, parent->size, parent->type);

  // get the dirent sector
  if(parent->type != 1) {
    dprintf("... error: parent inode is not directory\n");
    return -2; // parent not directory
  }
  int group = parent->size/DIRENTS_PER_SECTOR;
  char dirent_buffer[SECTOR_SIZE];
  if(group*DIRENTS_PER_SECTOR == parent->size) {
    // new disk sector is needed
    int newsec = bitmap_first_unused(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, SECTOR_BITMAP_SIZE);
    if(newsec < 0) {
      dprintf("... error: disk is full\n");
      return -1;
    }
    parent->data[group] = newsec;
    memset(dirent_buffer, 0, SECTOR_SIZE);
    dprintf("... new disk sector %d for dirent group %d\n", newsec, group);
  } else {
    if(Disk_Read(parent->data[group], dirent_buffer) < 0)
      return -1;
    dprintf("... load disk sector %d for dirent group %d\n", parent->data[group], group);
  }

  // add the dirent and write to disk
  int start_entry = group*DIRENTS_PER_SECTOR;
  offset = parent->size-start_entry;
  dirent_t* dirent = (dirent_t*)(dirent_buffer+offset*sizeof(dirent_t));
  strncpy(dirent->fname, file, MAX_NAME);
  dirent->inode = child_inode;
  if(Disk_Write(parent->data[group], dirent_buffer) < 0) return -1;
  dprintf("... append dirent %d (name='%s', inode=%d) to group %d, update disk sector %d\n", parent->size, dirent->fname, dirent->inode, group, parent->data[group]);

  // update parent inode and write to disk
  parent->size++;
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... update parent inode on disk sector %d\n", inode_sector);
  // dprintf("... new parent size =  %d\n", parent->size);
  
  return 0;
}

// used by both File_Create() and Dir_Create(); type=0 is file, type=1
// is directory
int create_file_or_directory(int type, char* pathname)
{
  int child_inode;
  char last_fname[MAX_NAME];
  int parent_inode = follow_path(pathname, &child_inode, last_fname);
  if(parent_inode >= 0) {
    if(child_inode >= 0) {
      dprintf("... file/directory '%s' already exists, failed to create\n", pathname);
      osErrno = E_CREATE;
      return -1;
    } else {
        if(add_inode(type, parent_inode, last_fname) >= 0) {
  	      dprintf("... successfully created file/directory: '%s'\n", pathname);
  	      return 0;
        } else {
  	      dprintf("... error: something wrong with adding child inode\n");
  	      osErrno = E_CREATE;
  	      return -1;
        }
      }
  } else {
    dprintf("... error: something wrong with the file/path: '%s'\n", pathname);
    osErrno = E_CREATE;
    return -1;
  }
}

// remove the child from parent; the function is called by both
// File_Unlink() and Dir_Unlink(); the function returns 0 if success,
// -1 if general error, -2 if directory not empty, -3 if wrong type
int remove_inode(int type, int parent_inode, int child_inode)
{
  /********* BEGING OUR CODE **********/
  //First we need to load the child inode sector 
  int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
 
  char inode_buffer[SECTOR_SIZE];
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);

  // get the child inode
  int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
 // printf("inode_start_entry = %d\n", inode_start_entry);

  int offset = child_inode-inode_start_entry;
  //printf("offset = %d\n", offset);

  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));

  //Now we need to check the child inode for errors
  if(child->type != type){    //If the type pass to the function does not match the child type
    return -3;                //ERROR -3 if wrong type
  }

  if(child->type == 1 && child->size > 0){    //If this inode is a directory and is not empty
    return -2;                                //ERROR -2 if directory not empty,
  }

  //Now we need to reclaim the data sectores of the child inode only if the inode is a file
  //if the inode is a directory is must already be empty (no data inside) in order to delete it
  if(child->type == 0){ //If I am deleting a file
    int i;
    for(i=0; i<MAX_SECTORS_PER_FILE; i++){   //Going through all the sectors 
        if(child->data[i] > 0){           //There is valid data in this sector that we need to clear
          bitmap_reset(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, child->data[i]);    //Clear the entry in the sector bitmap
          dprintf("... reseting bit sector %d from data index [%d] \n", child->data[i], i );
        }
      }
  }
  //At this point we are ready to delete the inode
  // Clear the child inode and write to disk

  memset(child, 0, sizeof(inode_t));

  
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("...  update disk sector %d\n", inode_sector);

  //Now we update the inode bitmap
  bitmap_reset(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, child_inode);

  //Now we need to update the parent inode
  // get the disk sector containing the parent inode
  inode_sector = INODE_TABLE_START_SECTOR+parent_inode/INODES_PER_SECTOR;
  if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... load inode table for parent inode %d from disk sector %d\n", parent_inode, inode_sector);

  // get the parent inode
  inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
  offset = parent_inode-inode_start_entry;
  assert(0 <= offset && offset < INODES_PER_SECTOR);
  inode_t* parent = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
  dprintf("... get parent inode %d (size=%d, type=%d)\n", parent_inode, parent->size, parent->type);

  // get the dirent sector
  if(parent->type != 1) {
    dprintf("... error: parent inode is not directory\n");
    return -2; // parent not directory
  }
  
  //Now we need to find in the parent inode the dirent structure that contains the child inode 
  //And then swap it with the last dirent entry in the parent inode and decrement size
   char dirent_buffer[SECTOR_SIZE];
    int group = 0;
    int entry = 0;    
    dirent_t* current_dirent;
  //First we are going to look for the last dirent entry in the parent inode
  


  if(parent->size > 1){  //There are more files or directories in the parent dierctory 
    int last_group = parent->size/DIRENTS_PER_SECTOR;
    char last_dirent_buffer[SECTOR_SIZE];
    int last_sector = parent->data[last_group];
    if(Disk_Read(last_sector, last_dirent_buffer) < 0)    //Read the sector used by the last entry
      return -1;
    dprintf("... load disk sector %d corresponding the last dirent entry in group %d\n", parent->data[last_group], group);

    //get the last dirent
    int start_entry = last_group*DIRENTS_PER_SECTOR;
    offset = parent->size-start_entry-1;
    dirent_t* last_dirent = (dirent_t*)(last_dirent_buffer+offset*sizeof(dirent_t));   //This is the last dirent to swap with the dirent that we are deleting
    //printf("-----------LAst dirent info is: name= %s and inode =%d and entry# = %d\n", last_dirent->fname, last_dirent->inode, offset);
    //Now find the sector where is the child dirent
    
    for(group = 0; group<MAX_SECTORS_PER_FILE; group++){       //Go through all the groups  data[group] in the parent inode
      if(Disk_Read(parent->data[group], dirent_buffer) < 0)    //Read the sector in this group
        return -1;
      dprintf("... load disk sector %d for dirent group %d\n", parent->data[last_group], group);
      for(entry = 0; entry<DIRENTS_PER_SECTOR; entry++){      //Go through all the dirents inside this group
         current_dirent = (dirent_t*)(dirent_buffer+entry*sizeof(dirent_t));
         if(current_dirent->inode == child_inode){
            //printf("-----------Before swaping the current dirent info is : name= %s and inode =%d\n", current_dirent->fname, current_dirent->inode);
            //current_dirent = last_dirent;
            strncpy(current_dirent->fname, last_dirent->fname, MAX_NAME);
            current_dirent->inode = last_dirent->inode; 

            char * blank = "";
            strncpy(last_dirent->fname, blank, MAX_NAME);
            last_dirent->inode = -1;
                     
            memset(last_dirent, 0 , sizeof(dirent_t));
            if(Disk_Write(parent->data[group], dirent_buffer) < 0) return -1;     //Here update the sector where we replace the removed node
            dprintf("... update dirent %d (name='%s', inode=%d) to group %d, update disk sector %d\n", (group*30)+entry, current_dirent->fname, current_dirent->inode, group, parent->data[group]);
            if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;     //Here update the parent inode sector
            group = MAX_SECTORS_PER_FILE; //just to break the outer loop
            break;
         }
      }
    }
  }

  // update parent inode and write to disk
  parent->size--;
  if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
  dprintf("... update parent inode on disk sector %d\n", inode_sector);
 
  return 0;
  /********* END OUR CODE **********/ 
  
}

// representing an open file
typedef struct _open_file {
  int inode; // pointing to the inode of the file (0 means entry not used)
  int size;  // file size cached here for convenience
  int pos;   // read/write position
} open_file_t;
static open_file_t open_files[MAX_OPEN_FILES];

// return true if the file pointed to by inode has already been open
int is_file_open(int inode)
{
  int i;
  for(i=0; i<MAX_OPEN_FILES; i++) {
    if(open_files[i].inode == inode)
      return 1;
  }
  return 0;
}

// return a new file descriptor not used; -1 if full
int new_file_fd()
{
  int i;
  for(i=0; i<MAX_OPEN_FILES; i++) {
    if(open_files[i].inode <= 0)
      return i;
  }
  return -1;
}

/* end of internal helper functions, start of API functions */

int FS_Boot(char* backstore_fname)
{
  dprintf("FS_Boot('%s'):\n", backstore_fname);
  // initialize a new disk (this is a simulated disk)
  if(Disk_Init() < 0) {
    dprintf("... disk init failed\n");
    osErrno = E_GENERAL;
    return -1;
  }
  dprintf("... disk initialized\n");
  
  // we should copy the filename down; if not, the user may change the
  // content pointed to by 'backstore_fname' after calling this function
  strncpy(bs_filename, backstore_fname, 1024);
  bs_filename[1023] = '\0'; // for safety
  
  // we first try to load disk from this file
  if(Disk_Load(bs_filename) < 0) {
    dprintf("... load disk from file '%s' failed\n", bs_filename);

    // if we can't open the file; it means the file does not exist, we
    // need to create a new file system on disk
    if(diskErrno == E_OPENING_FILE) {
      dprintf("... couldn't open file, create new file system\n");

      // format superblock
      char buf[SECTOR_SIZE];
      memset(buf, 0, SECTOR_SIZE);
      *(int*)buf = OS_MAGIC;
      if(Disk_Write(SUPERBLOCK_START_SECTOR, buf) < 0) {
	    dprintf("... failed to format superblock\n");
	    osErrno = E_GENERAL;
	    return -1;
      }

      dprintf("... formatted superblock (sector %d)\n", SUPERBLOCK_START_SECTOR);

      // format inode bitmap (reserve the first inode to root)
      bitmap_init(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, 1);
      dprintf("... formatted inode bitmap (start=%d, num=%d)\n", (int)INODE_BITMAP_START_SECTOR, (int)INODE_BITMAP_SECTORS);
      
      // format sector bitmap (reserve the first few sectors to
      // superblock, inode bitmap, sector bitmap, and inode table)
      bitmap_init(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS,DATABLOCK_START_SECTOR);
      dprintf("... formatted sector bitmap (start=%d, num=%d)\n",(int)SECTOR_BITMAP_START_SECTOR, (int)SECTOR_BITMAP_SECTORS);
      
      // format inode tables
      int i;
      for(i=0; i<INODE_TABLE_SECTORS; i++) {
	       memset(buf, 0, SECTOR_SIZE);
	       if(i==0) {
	       // the first inode table entry is the root directory
	         ((inode_t*)buf)->size = 0;
	         ((inode_t*)buf)->type = 1;
	       }
	       if(Disk_Write(INODE_TABLE_START_SECTOR+i, buf) < 0) {
  	        dprintf("... failed to format inode table\n");
  	        osErrno = E_GENERAL;
  	        return -1;	
          } 
        }

      dprintf("... formatted inode table (start=%d, num=%d)\n",(int)INODE_TABLE_START_SECTOR, (int)INODE_TABLE_SECTORS);
      
      // we need to synchronize the disk to the backstore file (so that we don't lose the formatted disk)
      if(Disk_Save(bs_filename) < 0) {
	     // if can't write to file, something's wrong with the backstore
      	dprintf("... failed to save disk to file '%s'\n", bs_filename);
      	osErrno = E_GENERAL;
      	return -1;
      } else {
      	// everything's good now, boot is successful
      	dprintf("... successfully formatted disk, boot successful\n");
      	memset(open_files, 0, MAX_OPEN_FILES*sizeof(open_file_t));
      	return 0;
      }
    } else {
      // something wrong loading the file: invalid param or error reading
      dprintf("... couldn't read file '%s', boot failed\n", bs_filename);
      osErrno = E_GENERAL; 
      return -1;
    }
  }else {
      dprintf("... load disk from file '%s' successful\n", bs_filename);
    
      // we successfully loaded the disk, we need to do two more checks,
      // first the file size must be exactly the size as expected (this
      // supposedly should be folded in Disk_Load(); and it's not)
      int sz = 0;
      FILE* f = fopen(bs_filename, "r");
      if(f) {
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fclose(f);
      }
      if(sz != SECTOR_SIZE*TOTAL_SECTORS) {
        dprintf("... check size of file '%s' failed\n", bs_filename);
        osErrno = E_GENERAL;
        return -1;
      }
      dprintf("... check size of file '%s' successful\n", bs_filename);
    
      // check magic
      if(check_magic()) {
        // everything's good by now, boot is successful
        dprintf("... check magic successful\n");
        memset(open_files, 0, MAX_OPEN_FILES*sizeof(open_file_t));
        return 0;
      } else {      
        // mismatched magic number
        dprintf("... check magic failed, boot failed\n");
        osErrno = E_GENERAL;
        return -1;
      }
  }
}

int FS_Sync()
{
  if(Disk_Save(bs_filename) < 0) {
    // if can't write to file, something's wrong with the backstore
    dprintf("FS_Sync():\n... failed to save disk to file '%s'\n", bs_filename);
    osErrno = E_GENERAL;
    return -1;
  } else {
    // everything's good now, sync is successful
    dprintf("FS_Sync():\n... successfully saved disk to file '%s'\n", bs_filename);
    return 0;
  }  
}

int File_Create(char* file)
{
  dprintf("File_Create('%s'):\n", file);
  return create_file_or_directory(0, file);
}

int File_Unlink(char* file)
{
  /* YOUR CODE */
  dprintf("File_Unlink('%s'):\n", file);
  
  int child_inode;
  char last_fname[MAX_NAME];
  int parent_inode = follow_path(file, &child_inode, last_fname);   //Get the father inode
  
  if(parent_inode >= 0) {         //Father inode found 
    if(child_inode >= 0) {        //Child inode found
      
      if(is_file_open(child_inode)==1){     //File is open
      osErrno = E_FILE_IN_USE;    
      return -1;    
      }
      
      int result;
      result  = remove_inode(0, parent_inode, child_inode); //Succefully remove the inode representing a file
      
      switch(result){// -1 if general error, -2 if directory not empty, -3 if wrong type
        case 0:   dprintf("... Succefully remove the inode representing a file\n");
                  return 0;

        case -1:  dprintf("... General error Removing the inode\n");
                  osErrno = E_GENERAL;
                  return -1;

        case -2:  dprintf("... Current directory is not empty\n");
                  osErrno = E_DIR_NOT_EMPTY;
                  return -1;

        case -3:  dprintf("... Wrong type\n");
                  osErrno = E_GENERAL;
                  return -1;
      }
           

    }else{
      dprintf("... file '%s' does not exists, failed to delete\n", file);
      osErrno = E_NO_SUCH_FILE;
      return -1;
    }  
  } else {
      dprintf("... error: something wrong with the file/path: '%s'\n", file);
      osErrno = E_NO_SUCH_FILE;
      return -1;
  }
  return -1;
    //End Our code
}

int File_Open(char* file)
{
  dprintf("File_Open('%s'):\n", file);
  int fd = new_file_fd();
  if(fd < 0) {
    dprintf("... max open files reached\n");
    osErrno = E_TOO_MANY_OPEN_FILES;
    return -1;
  }

  int child_inode;
  follow_path(file, &child_inode, NULL);
  if(child_inode >= 0) { // child is the one
    // load the disk sector containing the inode
    int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
    char inode_buffer[SECTOR_SIZE];
    if(Disk_Read(inode_sector, inode_buffer) < 0) { osErrno = E_GENERAL; return -1; }
    dprintf("... load inode table for inode from disk sector %d\n", inode_sector);

    // get the inode
    int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = child_inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
    dprintf("... inode %d (size=%d, type=%d)\n",
	    child_inode, child->size, child->type);

    if(child->type != 0) {
      dprintf("... error: '%s' is not a file\n", file);
      osErrno = E_GENERAL;
      return -1;
    }

    // initialize open file entry and return its index
    open_files[fd].inode = child_inode;
    open_files[fd].size = child->size;
    open_files[fd].pos = 0;
    return fd;
  } else {
    dprintf("... file '%s' is not found\n", file);
    osErrno = E_NO_SUCH_FILE;
    return -1;
  }  
}

int File_Read(int fd, void* buffer, int size)
{
  /* YOUR CODE */
  dprintf("... Reading File \n");
  int bufCounter;
  int i;
 // dprintf("... open_files.nodes = %d \n", open_files[fd].inode);
  for(i=0; i<MAX_OPEN_FILES; i++) { 
	if(fd==i){
		if(is_file_open(open_files[fd].inode) == 1){ //going through open_file array to check if file is open
			break;
			
		
		}
		else{
			
			osErrno=E_BAD_FD;
		
			return -1;
			
		}
		
		

	}
	else{
		
		osErrno=E_BAD_FD; //if the fd is never found in the max open files
		return -1;
	
	}
  }	
  dprintf("... open_files.nodes = %d \n", open_files[fd].inode);
	int toRead=(open_files[fd].pos + size);//position of where read is to end
	if(toRead > open_files[fd].size){ //if reading is bigger than the file size, we'll read until the end of the file
		toRead=(open_files[fd].size);
	}

		
	//getting child inode
	int child_inode=open_files[fd].inode;
		
	int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
 
	char inode_buffer[SECTOR_SIZE];
	if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
		dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);

	// get the point where to start reading the data from
	int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
	int offset = child_inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
	dprintf("... inode %d (size=%d, type=%d)\n",child_inode, child->size, child->type);
	char buf[SECTOR_SIZE]; //buffer to receive all the data of the file
	int p=0; //counter from the position of the file open pointer that will increase to the end
	int u=toRead; //counter from the end of where you are supposed to read going down until you reach the same number as the original pointer position
	for(i = 0; i<MAX_SECTORS_PER_FILE; i++){       //Go through all the file data 
      if(Disk_Read(child->data[i], buf) == 0){ //get data from disk
		  int j;
		  for(j=0;j<SECTOR_SIZE;j++){ //read the sector that is loaded
			  if(p>=open_files[fd].pos && u>=open_files[fd].pos){ //if data is within pointer and ending, write to buffer
					buffer[bufCounter+j]=buf[j]; //having problem with the void pointer dereferencing
					bufCounter+=j;
					p++;
					u--;
			  }
		  }
		  
	   }else{
		   dprintf("... file read error \n");
			return -1;
	   }
	   
	}
	
	
	
	
  
  
  return toRead;
}

int File_Write(int fd, void* buffer, int size)
{
  /* YOUR CODE */
  return -1;
}

int File_Seek(int fd, int offset)
{
  /* YOUR CODE */
  int i;
  for(i=0; i<MAX_OPEN_FILES; i++) { 
	if(fd==i){
		if(is_file_open( open_files[fd].inode) == 0){ //going through open_file array to check if file is open
      
			osErrno=E_BAD_FD;
		
			return -1;
		
		}
		

	}
	else{
		
		osErrno=E_BAD_FD; //if the fd is never found in the max open files
		return -1;
	
	}
  }	
	if(open_files[fd].size>offset || open_files[fd].size<0){
		
		osErrno = E_SEEK_OUT_OF_BOUNDS;
		return -1;
	}
  
	open_files[fd].pos=offset;
	
	return open_files[fd].pos;
  
  
  return 0;
}

int File_Close(int fd)
{
  dprintf("File_Close(%d):\n", fd);
  if(0 > fd || fd > MAX_OPEN_FILES) {
    dprintf("... fd=%d out of bound\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }
  if(open_files[fd].inode <= 0) {
    dprintf("... fd=%d not an open file\n", fd);
    osErrno = E_BAD_FD;
    return -1;
  }

  dprintf("... file closed successfully\n");
  open_files[fd].inode = 0;
  return 0;
}

int Dir_Create(char* path)
{
  dprintf("Dir_Create('%s'):\n", path);
  return create_file_or_directory(1, path);
}

int Dir_Unlink(char* path)
{
  /* YOUR CODE */
  
   /* YOUR CODE */
  dprintf("Dir_Unlink('%s'):\n", path);
  
  int child_inode;
  char last_fname[MAX_NAME];
  int parent_inode = follow_path(path, &child_inode, last_fname);   //Get the father inode
  
  if(parent_inode >= 0) {         //Father inode found 
    if(child_inode >= 0) {        //Child inode found
      
      
      int result;
      result  = remove_inode(1, parent_inode, child_inode); //Succefully remove the inode representing a file
      
      switch(result){// -1 if general error, -2 if directory not empty, -3 if wrong type
        case 0:   dprintf("... Succefully remove the inode representing a Dir\n");
                  return 0;

        case -1:  dprintf("... General error Removing the inode\n");
                  osErrno = E_GENERAL;
                  return -1;

        case -2:  dprintf("... Current directory is not empty\n");
                  osErrno = E_DIR_NOT_EMPTY;
                  return -1;

        case -3:  dprintf("... Wrong type\n");
                  osErrno = E_GENERAL;
                  return -1;
      }
           

    }else{
      dprintf("... Directory '%s' does not exists, failed to delete\n", path);
      osErrno = E_NO_SUCH_DIR;
      return -1;
    }  
  } else {
      dprintf("... error: something wrong with the directory/path: '%s'\n", path);
      osErrno = E_NO_SUCH_DIR;
      return -1;
  }
  return -1;
  
  
  
  
  
  return -1;
}

int Dir_Size(char* path)
{
  /* YOUR CODE */
  return 0;
}

int Dir_Read(char* path, void* buffer, int size)
{
  /* YOUR CODE */
  return -1;
}
