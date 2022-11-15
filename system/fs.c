#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>


static fsystem_t fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];


#define SB_BLK 0 // Superblock
#define BM_BLK 1 // Bitmapblock

#define NUM_FD 16

filetable_t oft[NUM_FD]; // open file table
#define isbadfd(fd) (fd < 0 || fd >= NUM_FD || oft[fd].in.id == EMPTY)

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(inode_t))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

/**
 * Helper functions
 */
int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEDIRECTBLOCKS) {
    errormsg("No indirect block support! (%d >= %d)\n", fileblock, INODEBLOCKS - 2);
    return SYSERR;
  }

  // Get the logical block address
  diskblock = oft[fd].in.blocks[fileblock];

  return diskblock;
}

/**
 * Filesystem functions
 */
int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out) {
  int bl, inn;
  int inode_off;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %s)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl  = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(inode_t);

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(out, &block_cache[inode_off], sizeof(inode_t));

  return OK;

}

int _fs_put_inode_by_num(int dev, int inode_number, inode_t *in) {
  int bl, inn;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number >= fsd.ninodes) {
    errormsg("inode %d out of range (> %d)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(inode_t))], in, sizeof(inode_t));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes) {
  int i;

  if (dev == dev0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  } else {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  } else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) { i++; }
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *) SYSERR) {
    errormsg("fs_mkfs memget failed\n");
    return SYSERR;
  }

  /* zero the free mask */
  for(i = 0; i < fsd.freemaskbytes; i++) {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(fsystem_t));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  // Initialize all inode IDs to EMPTY
  inode_t tmp_in;
  for (i = 0; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    tmp_in.id = EMPTY;
    _fs_put_inode_by_num(dev0, i, &tmp_in);
  }
  fsd.root_dir.numentries = 0;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
  }

  for (i = 0; i < NUM_FD; i++) {
    oft[i].state     = 0;
    oft[i].fileptr   = 0;
    oft[i].de        = NULL;
    oft[i].in.id     = EMPTY;
    oft[i].in.type   = 0;
    oft[i].in.nlink  = 0;
    oft[i].in.device = 0;
    oft[i].in.size   = 0;
    memset(oft[i].in.blocks, 0, sizeof(oft[i].in.blocks));
    oft[i].flag      = 0;
  }

  return OK;
}

int fs_freefs(int dev) {
  if (freemem(fsd.freemask, fsd.freemaskbytes) == SYSERR) {
    return SYSERR;
  }

  return OK;
}

/**
 * Debugging functions
 */
void fs_print_oft(void) {
  int i;

  printf ("\n\033[35moft[]\033[39m\n");
  printf ("%3s  %5s  %7s  %8s  %6s  %5s  %4s  %s\n", "Num", "state", "fileptr", "de", "de.num", "in.id", "flag", "de.name");
  for (i = 0; i < NUM_FD; i++) {
    if (oft[i].de != NULL) printf ("%3d  %5d  %7d  %8d  %6d  %5d  %4d  %s\n", i, oft[i].state, oft[i].fileptr, oft[i].de, oft[i].de->inode_num, oft[i].in.id, oft[i].flag, oft[i].de->name);
  }

  printf ("\n\033[35mfsd.root_dir.entry[] (numentries: %d)\033[39m\n", fsd.root_dir.numentries);
  printf ("%3s  %3s  %s\n", "ID", "id", "filename");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (fsd.root_dir.entry[i].inode_num != EMPTY) printf("%3d  %3d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
  printf("\n");
}

void fs_print_inode(int fd) {
  int i;

  printf("\n\033[35mInode FS=%d\033[39m\n", fd);
  printf("Name:    %s\n", oft[fd].de->name);
  printf("State:   %d\n", oft[fd].state);
  printf("Flag:    %d\n", oft[fd].flag);
  printf("Fileptr: %d\n", oft[fd].fileptr);
  printf("Type:    %d\n", oft[fd].in.type);
  printf("nlink:   %d\n", oft[fd].in.nlink);
  printf("device:  %d\n", oft[fd].in.device);
  printf("size:    %d\n", oft[fd].in.size);
  printf("blocks: ");
  for (i = 0; i < INODEBLOCKS; i++) {
    printf(" %d", oft[fd].in.blocks[i]);
  }
  printf("\n");
  return;
}

void fs_print_fsd(void) {
  int i;

  printf("\033[35mfsystem_t fsd\033[39m\n");
  printf("fsd.nblocks:       %d\n", fsd.nblocks);
  printf("fsd.blocksz:       %d\n", fsd.blocksz);
  printf("fsd.ninodes:       %d\n", fsd.ninodes);
  printf("fsd.inodes_used:   %d\n", fsd.inodes_used);
  printf("fsd.freemaskbytes  %d\n", fsd.freemaskbytes);
  printf("sizeof(inode_t):   %d\n", sizeof(inode_t));
  printf("INODES_PER_BLOCK:  %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS:  %d\n", NUM_INODE_BLOCKS);

  inode_t tmp_in;
  printf ("\n\033[35mBlocks\033[39m\n");
  printf ("%3s  %3s  %4s  %4s  %3s  %4s\n", "Num", "id", "type", "nlnk", "dev", "size");
  for (i = 0; i < NUM_FD; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) printf("%3d  %3d  %4d  %4d  %3d  %4d\n", i, tmp_in.id, tmp_in.type, tmp_in.nlink, tmp_in.device, tmp_in.size);
  }
  for (i = NUM_FD; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) {
      printf("%3d:", i);
      int j;
      for (j = 0; j < 64; j++) {
        printf(" %3d", *(((char *) &tmp_in) + j));
      }
      printf("\n");
    }
  }
  printf("\n");
}

void fs_print_dir(void) {
  int i;

  printf("%22s  %9s  %s\n", "DirectoryEntry", "inode_num", "name");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    printf("fsd.root_dir.entry[%2d]  %9d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
}

void fs_print_dirs(directory_t *working_dir, int level) {

  int i, j;

  if (working_dir == NULL) {
    working_dir = &(fsd.root_dir);
    level = 0;
    printf("\nroot\n");
  }

  inode_t _in;
  directory_t cache;

  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (working_dir->entry[i].inode_num != EMPTY) {

      _fs_get_inode_by_num(dev0, working_dir->entry[i].inode_num, &_in);

      for (j = 0; j <= level; j++) printf("\033[38;5;238m:\033[39m ");
      if (_in.type == INODE_TYPE_FILE) {
        printf("f %s\n", working_dir->entry[i].name);
      } else {
        printf("d %s\n", working_dir->entry[i].name);
        bs_bread(dev0, _in.blocks[0], 0, &cache, sizeof(directory_t));
        if (cache.numentries > 0) fs_print_dirs(&cache, level + 1);
      }
    }
  }
}

int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
}

int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/**
 * This is maybe a little overcomplicated since the lowest-numbered
 * block is indicated in the high-order bit.  Shift the byte by j
 * positions to make the match in bit7 (the 8th bit) and then shift
 * that value 7 times to the low-order bit to print.  Yes, it could be
 * the other way...
 */
void fs_printfreemask(void) { // print block bitmask
  int i, j;

  for (i = 0; i < fsd.freemaskbytes; i++) {
    for (j = 0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    printf(" ");
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}


/**
 * TODO: implement the functions below
 */
int fs_open(char *filename, int flags){
    // Flags that should be covered are O_RDONLY, O_WRONLY, and O_RDWR. If not, return SYSERR
    // Search for a file name by iterating through the root directory
    // Return SYSERR if already open
    // Using _fs_get_inode_by_num function, make an entry of that inode in the file table. (Only the inodes in the file table can be edited)
    // Return file descriptor on success

    // If flags are not O_RDONLY, or O_WRONLY, or O_RDONLY, return SYSERR
    // if(!(flags == O_RDONLY || flags == O_WRONLY || flags == O_RDONLY)){
    //     return SYSERR;
    // }
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        // Since there is no duplicated file created, once match the file name, start open file
        if(strncmp(&(fsd.root_dir.entry[i].name[0]), filename, FILENAMELEN) == 0){
            // index -> file descriptor
            int index = -1;
            for(int j = 0; j < NUM_FD; j++){
                if(oft[j].de == NULL){
                    if(index == -1){
                        index = j;
                    }
                }
                if(strncmp(&(oft[j].de->name[0]), filename, FILENAMELEN) == 0){
                    // If file is FSTATE_CLOSED, return current index as file descriptor
                    if(oft[j].state == FSTATE_CLOSED){
                        oft[j].flag = flags;
                        oft[j].state = FSTATE_OPEN;
                        return j;
                    }
                    // Else, file is already opened, return SYSERR
                    else{
                        return SYSERR;
                    }
                }
            }
            // If directory is full, return SYSERR
            if(index == -1){
                return SYSERR;
            }
            // // Create a new node and initialize all the status of this new node
            // struct inode node;
            // // Init 5 status: size, device, nlink, type, id
            // node.size = 0;
            // node.device = 0;
            // node.nlink = 0;
            // node.type = 0;
            // node.id = 0;
            // // Allocate memory for the block
            // memset(node.blocks, EMPTY, sizeof(*node.blocks) * INODEBLOCKS);

            struct inode node;
            // Read inode from the block store
            _fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &node);
            struct dirent *ofdep = (struct dirent*)getmem(sizeof(dirent_t));
            // Link ofdep to fsd.root_dir.entry[i]
            ofdep->inode_num = fsd.root_dir.entry[i].inode_num;
            // Change ofdep.filename
            strcpy(&ofdep->name[0], filename);
            // Change oft status: state, fileptr, de, in, flag
            oft[index].state = FSTATE_OPEN;
            oft[index].fileptr = 0;
            oft[index].de = ofdep;
            oft[index].in = node;
            oft[index].flag = flags;
            // Open success, return file descriptor
            return index;
        }
    }
    return SYSERR;
}

int fs_close(int fd) {
    // Return SYSERR if already closed
    // Change the state of an open file to FSTATE_CLOSED in the file table
    // Return OK on success

    // If file already closed, return SYSERR;
    if(oft[fd].state == FSTATE_CLOSED){
        return SYSERR;
    }
    // Else turn file state to FSTATE_CLOSED
    else{
        oft[fd].state = FSTATE_CLOSED;
    }
    // Return OK on success
    return OK;
}

int fs_create(char *filename, int mode){
    // This function only supports the following mode as defined in include/fs.h
    //    File creation : INODE_TYPE_FILE 1
    //    Directory creations (besides the existing root directory) are not in scope for this assignment
    // The root directory is defined in one of the structs above which you can easily find. When a new file or directory is created, the root directory is updated accordingly.
    // Return SYSERR if root directory is already full
    // Return SYSERR if the filename already exists (no duplicate filenames are allowed)
    // Methodology
    //    Determine next available inode number (take inode deletion into account)
    //    Add inode to the file system i.e. fsd data structure
    //    Open the file with O_RDWR flags.
    // Return file descriptor on success
    int index = -1;
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        // Compare names, if same name, return SYSERR
        if(strncmp(&(fsd.root_dir.entry[i].name[0]), filename, FILENAMELEN) == 0){
            return SYSERR;
        }
        if(fsd.root_dir.entry[i].inode_num == EMPTY){
            if(index == -1){
                index = i;
            }
        }
    }
    if(index == -1){
        return SYSERR;
    }
    int nodeIndex = index;
    // Create new inode and initialize the node
    struct inode* node = (struct inode*)getmem(sizeof(inode_t));
    // Init 5 status: size, device, nlink, type, id
    node->size = 0;
    node->device = dev0;
    node->nlink = 1;
    node->type = INODE_TYPE_FILE;
    node->id = nodeIndex;
    // node->id = -1;
    // inode_t tempNode;
    // for(int i = 0; i < fsd.ninodes; i++){
    //     _fs_get_inode_by_num(dev0, i, &tempNode);
    //     if(tempNode.id == EMPTY){
    //         tempNode.id = i;
    //         break;
    //     }
    // }
    // node->id = tempNode.id;
    // // No inode available
    // if(node->id == -1){
    //     return SYSERR;
    // }
    fs_setmaskbit(nodeIndex + 2);


    // Allocate memory needed for the block of the nodes to store
    memset(node->blocks, EMPTY, sizeof(*node->blocks)*INODEBLOCKS);
    // Write inodes to the block store
    _fs_put_inode_by_num(dev0, nodeIndex, node);

    fsd.root_dir.entry[index].inode_num = node->id;
    strcpy(&fsd.root_dir.entry[index].name[0], filename);
    fsd.root_dir.numentries++;
    fsd.inodes_used++;

    // // Allocate memory needed for direct_t
    // struct dirent *ofdep = (struct dirent*)getmem(sizeof(dirent_t));
    // ofdep->inode_num = nodeIndex;
    // // Change ofdep.filename, numentries needs to be plus 1, and link the location node to ofdep
    // strcpy(&(ofdep->name[0]), filename);
    // fsd.root_dir.numentries++;
    // fsd.root_dir.entry[index] = *ofdep;


    // Return file descriptor on success
    // Once create file succeed, open file with flag == O_RDWR
    return fs_open(filename, O_RDWR);
}

int fs_seek(int fd, int offset) {
    // The offset is to be used as the absolute position of the file pointer (no relative position change)
    // Return SYSERR if the offset would go out of bounds
    // Return OK on success

    // // offset smaller than lower bound
    // if(offset < 0){
    //     return SYSERR;
    // }
    // // offset bigger or equal to upper bound
    // if(offset >= oft[fd].in.size){
    //     return SYSERR;
    // }
    // // If isbadfd, return SYSERR
    // if(isbadfd(fd)){
    //     return SYSERR;
    // }
    // // Can only seek open files
    // if(oft[fd].state == FSTATE_OPEN){
    //     // Set the fileptr to offset
    //     oft[fd].fileptr = offset;
    //     return OK;
    // }

    // Collection of SYSERR cases
    if(offset < 0 || offset > oft[fd].in.size || isbadfd(fd) || oft[fd].state == FSTATE_CLOSED){
        return SYSERR;
    }
    oft[fd].fileptr = offset;
    return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
    // Read file contents stored in the data blocks
    // Return the number of bytes read or SYSERR

    // // If isbadfd, return SYSERR
    // if(isbadfd(fd)){
    //     return SYSERR;
    // }
    // // if write only, return SYSERR
    // if(oft[fd].flag == O_WRONLY){
    //     return SYSERR;
    // }
    // if(nbytes < 0){
    //     return SYSERR;
    // }

    // Collections of SYSERR cases
    if(isbadfd(fd) || oft[fd].flag == O_WRONLY || nbytes < 0 || oft[fd].state == FSTATE_CLOSED){
        return SYSERR;
    }
    // Create a temporary buffer as a counter of buf
    for(int i = 0; i < nbytes; i++){
        if(oft[fd].in.size >= oft[fd].fileptr){
            // Use bs_bread to read from block
            bs_bread(dev0, oft[fd].in.blocks[oft[fd].fileptr / MDEV_BLOCK_SIZE], oft[fd].fileptr % MDEV_BLOCK_SIZE, buf, sizeof(byte));
            // Update counter and fileptr;
            buf++;
            oft[fd].fileptr++;
        }
        else{
            break;
        }
    }
    return nbytes;
}

int fs_write(int fd, void *buf, int nbytes) {
    // Write content to the data blocks of the file
    // Allocate new blocks if needed (do not exceed the maximum limit)
    // Do not forget to update size and fileptr
    // Return the number of bytes written or SYSERR
    
    // // If isbadfd, return SYSERR
    // if(isbadfd(fd)){
    //     return SYSERR;
    // }
    // // If read only, return SYSERR
    // if(oft[fd].flag == O_RDONLY){
    //     return SYSERR;
    // }
    // if(nbytes < 0){
    //     return SYSERR;
    // }
    
    // Collections of SYSERR cases
    if(isbadfd(fd) || oft[fd].flag == O_RDONLY || nbytes < 0 || oft[fd].state == FSTATE_CLOSED){
        return SYSERR;
    }
    for(int i = 0; i < nbytes; i++){
        if(oft[fd].fileptr == oft[fd].in.size){
            int blockIndex = -1, blockInodeIndex = -1, j;
            for(j = 18; j < MDEV_BLOCK_SIZE; j++){
            // for(j = 0; j < MDEV_BLOCK_SIZE; j++){
                // Get the first free block to use
                if(fs_getmaskbit(j) != 1){
                    blockIndex = j;
                    fs_setmaskbit(j);
                    break;
                }
            }
            // If no available block to use, return SYSERR
            if(blockIndex == -1){
                return SYSERR;
            }
            for(j = 0; j < INODEBLOCKS; j++){
                // Get the first free space for storing the block
                if(oft[fd].in.blocks[j] == EMPTY){
                    oft[fd].in.blocks[j] = blockIndex;
                    blockInodeIndex = j;
                    break;
                }
            }
            // If no free space, return SYSERR
            if(blockInodeIndex == -1){
                return SYSERR;
            }
            // Write too many: file size should not exceed the filesystem size
            if(oft[fd].in.size > (INODEDIRECTBLOCKS - 1) * MDEV_BLOCK_SIZE){
                return i;
            }
            // Update block size
            oft[fd].in.size = oft[fd].in.size + MDEV_BLOCK_SIZE;
        }
        // Start writing
        bs_bwrite(dev0, oft[fd].in.blocks[oft[fd].fileptr / MDEV_BLOCK_SIZE], oft[fd].fileptr % MDEV_BLOCK_SIZE, buf, sizeof(byte));
        // Update buf and fileptr
        buf++;
        oft[fd].fileptr++;
    }
    return nbytes;
}

int fs_link(char *src_filename, char* dst_filename) {
    // Search the src_filename in the root directory
    // No duplicate link names are allowed
    // Copy the inode num of src_filename to the new entry with dst_filename as its filename
    // Return OK on success or else SYSERR
    
    // Check if one of the both input char* pointer is NULL
    if(src_filename == NULL || dst_filename == NULL){
        return SYSERR;
    }

    // If src_filename is same as dst_filename, return SYSERR (duplicate filename)
    if(strncmp(src_filename, dst_filename, FILENAMELEN) == 0){
        return SYSERR;
    }
    int srcFilePos = -1, dstFilePos = -1;
    // Try to find src_file in root_dir
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        if(strncmp(&fsd.root_dir.entry[i].name[0], src_filename, FILENAMELEN) == 0){
            srcFilePos = i;
            break;
        }
    }
    // If src_file not found in root_dir, return SYSERR
    if(srcFilePos == -1){
        return SYSERR;
    }
    // Try to find dst_file in root_dir
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        if(strncmp(&fsd.root_dir.entry[i].name[0], dst_filename, FILENAMELEN) == 0){
            dstFilePos = i;
            break;
        }
    }
    // If root_dir contains the same filename as dst_file, return SYSER
    if(dstFilePos != -1){
        return SYSERR;
    }
    // Get the inode num of src_file in root_dir
    int srcInodeNum = -1;
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        if(strncmp(&fsd.root_dir.entry[i].name[0], src_filename, FILENAMELEN) == 0){
            srcInodeNum = fsd.root_dir.entry[i].inode_num;
            break;
        }
    }
    // If No valid inode_num for src_file
    if(srcInodeNum == -1){
        return SYSERR;
    }

    // If numentries equals to DIRECTORY_SIZE, all entries are being occupied, return SYSERR
    // Need to check this one?
    // if(fsd.root_dir.numentries == DIRECTORY_SIZE){
    //     return SYSERR;
    // }
    // Find the available place for new entry
    int availableEntryPos = -1;
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        if(fsd.root_dir.entry[i].inode_num == EMPTY){
            availableEntryPos = i;
            break;
        }
    }
    // All entries are being occupied
    if(availableEntryPos == -1){
        return SYSERR;
    }
    
    // Insert the file into the available entry slot (FOR FSD)
    fsd.root_dir.entry[availableEntryPos].inode_num = srcInodeNum;
    strcpy(&fsd.root_dir.entry[availableEntryPos].name[0], dst_filename);
    // Update numentries and inodes_used number
    fsd.root_dir.numentries++;
    fsd.inodes_used++;
    // Create an inode pointer to make change on nlink (FOR INODE)
    struct inode* tempNode = (struct inode*)getmem(sizeof(inode_t));
    _fs_get_inode_by_num(dev0, fsd.root_dir.entry[availableEntryPos].inode_num, tempNode);
    tempNode->nlink++;
    _fs_put_inode_by_num(dev0, fsd.root_dir.entry[availableEntryPos].inode_num, tempNode);

    return OK;
}

int fs_unlink(char *filename) {
    // Search filename in the root directory
    // If the nlinks of the respective inode is more than 1, just remove the entry in the root directory
    // If the nlinks of the inode is just 1, then delete the respective inode along with its data blocks as well
    // Return OK on success or else SYSERR

    // Working stage:
    // 1. Find the target file in file system
    // 2. Get corresponding inode_num
    // 3. Check inode 

    int index = -1;
    // Iterative serach the filename in the directory
    for(int i = 0; i < DIRECTORY_SIZE; i++){
        if(strncmp(&fsd.root_dir.entry[i].name[0], filename, FILENAMELEN) == 0){
            index = i;
        }
    }
    // If not find file in directory, return SYSERR
    if(index == -1){
        return SYSERR;
    }

    // Create an inode pointer and get inode from the target file
    struct inode* tempNode = (struct inode*)getmem(sizeof(inode_t));
    _fs_get_inode_by_num(dev0, fsd.root_dir.entry[index].inode_num, tempNode);
    // Condition 1: Target file has only 1 link
    if(tempNode->nlink == 1){
        // Clear all the maskbit which are being occupied by the file (FOR BLOCKS)
        for(int i = 0; i < INODEDIRECTBLOCKS; i++){
            fs_clearmaskbit(tempNode->blocks[i]);
        }
        // Change the inode status, and update it (FOR INODE)
        tempNode->nlink = 0;
        // tempNode->id = EMPTY;
        // Free the blocks being used by this inode
        for(int i = 0; i < INODEBLOCKS; i++){
            tempNode->blocks[i] = EMPTY;
        }
        // Find the corresponding block which stores this INODE, then clear the block
        // fs_clearmaskbit(tempNode->id / INODES_PER_BLOCK + 2);
        // fs_clearmaskbit(tempNode->id + 2);

        tempNode->id = EMPTY;
        _fs_put_inode_by_num(dev0, fsd.root_dir.entry[index].inode_num, tempNode);
        // Change the current entry status (FOR FSD)
        fsd.root_dir.entry[index].inode_num = EMPTY;
        strcpy(&fsd.root_dir.entry[index].name[0], "");
        // Release 1 occupied inodes_used number (FOR FSD)
        fsd.inodes_used--;
        // Release 1 occupied entry slot (FOR FSD)
        fsd.root_dir.numentries--;
    }
    // Condition 2: Target file has more than 1 link
    else if(tempNode->nlink > 1){
        // Decrease the link number by 1, and update it (FOR INODE)
        tempNode->nlink--;
        _fs_put_inode_by_num(dev0, fsd.root_dir.entry[index].inode_num, tempNode);
        // Change the current entry status (FOR FSD)
        fsd.root_dir.entry[index].inode_num = EMPTY;
        strcpy(&fsd.root_dir.entry[index].name[0], "");
        // Release 1 occupied entry slot (FOR FSD)
        fsd.root_dir.numentries--;
    }
    // Condition 3: Target file has no link, return SYSERR
    else{
        return SYSERR;
    }
    return OK;
}

#endif /* FS */
