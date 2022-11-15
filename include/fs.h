#ifndef FS_H
#define FS_H

/* #define FS_DEBUG */
#ifdef FS_DEBUG
#if __GNUC__ > 7
#define errormsg(fmt, ...) printf("\033[31mERROR %20s:%-3d %30s()\033[39m " fmt, __FILE__, __LINE__,  __func__ __VA_OPT__(,) __VA_ARGS__);
#else
#define errormsg(fmt, ...) printf("\033[31mERROR %20s:%-3d %30s()\033[39m " fmt, __FILE__, __LINE__,  __func__, ##__VA_ARGS__);
#endif
#else
#define errormsg(fmt, ...)
#endif

/* Modes for file creation */
#define O_CREAT 11
#define O_DIR   12

/* Flags of file */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

#define FILENAMELEN 16
#define INODEBLOCKS 12
#define INODEDIRECTBLOCKS (INODEBLOCKS - 2)
#define DIRECTORY_SIZE 16

#define MDEV_BLOCK_SIZE 512
#define MDEV_NUM_BLOCKS 512
#define DEFAULT_NUM_INODES (MDEV_NUM_BLOCKS / 4)

#define INODE_TYPE_FILE 1
#define INODE_TYPE_DIR 2

/**
 * File states to check if file is open or closed
 */
#define FSTATE_CLOSED 0
#define FSTATE_OPEN 1

/**
 * Structure of inode
 */
typedef struct inode {
  int id;                   // !< Inode id: file descriptor
  short int type;           // !< INODE_TYPE_FILE, INODE_TYPE_DIR
  short int nlink;          // !< Number of hard links
  int device;               // !< Device
  int size;
  int blocks[INODEBLOCKS];  // !< First INODEBLOCKS which contain file's data
} inode_t;


/**
 * Struct to store file details like state, fileptr
 */
typedef struct filetable {
  int state;          // !< State: FSTATE_OPEN, FSTATE_CLOSED
  int fileptr;        // !< file pointer offset
  struct dirent *de;  // !< Directory entry
  inode_t in;         // !< Inode structure
  int flag;           // !< Contains the permission
} filetable_t;

/**
 * Struct to store directory entry
 */
typedef struct dirent {
  int inode_num;           // !< Inode number
  char name[FILENAMELEN];  // !< Filename
} dirent_t;

/**
 * Struct to store directory details
 */
typedef struct directory {
  int numentries;                  // !< Number of entries
  dirent_t entry[DIRECTORY_SIZE];  // !< Subdirectories
} directory_t;

/**
 * Struct to file system details
 */
typedef struct fsystem {
  int nblocks;           // !< Number of blocks
  int blocksz;           // !< Block size
  int ninodes;           // !< Number of inodes
  int inodes_used;       // !< Number of inodes in use
  int freemaskbytes;     // !< Number of bytes for free bitmap
  char *freemask;        // !< Free bitmap
  directory_t root_dir;  // !< Root directory (entry point into fs)
} fsystem_t;


/**
 * File and directory functions
 */

/**
 * fs_open
 * Open a file
 *
 * @param     filename
 * @param     flags
 * @returns   file descriptor on success or else SYSERR
 */
int fs_open(char *filename, int flags);

/**
 * fs_close
 * Close a file pointed by its file descriptor @fd
 * Closing an already closed file is not allowed
 *
 * @param     fd
 * @returns   OK on success or else SYSERR
 */
int fs_close(int fd);

/**
 * fs_create
 * Create a file with name @filename and @mode
 * This also opens the file
 *
 * @param     filename
 * @param     mode
 * @returns   OK on success or else SYSERR
 */
int fs_create(char *filename, int mode);

/**
 * fs_seek
 * Change file pointer position within a given file @fd
 * Offset must be within the bounds of the file
 *
 * @param     fd       file descriptor of file to change file pointer of
 * @param     offset   offset into file
 * @returns   OK on success or else SYSERR
 */
int fs_seek(int fd, int offset);

/**
 * fs_read
 * Read from file @fd to buffer @buf
 *
 * @param     fd       file descriptor of file to read from
 * @param     buf      destination buffer
 * @param     nbytes   number of bytes to read
 * @returns   OK on success or else SYSERR
 */
int fs_read(int fd, void *buf, int nbytes);

/**
 * fs_write
 * Write from buffer @buf to file @fd
 * Allocate new data blocks if needed
 *
 * @param     fd       file descriptor of file to write to
 * @param     buf      source buffer
 * @param     nbytes   number of bytes to write
 * @returns   bytes written
 */
int fs_write(int fd, void *buf, int nbytes);

/**
 * fs_link
 * Add a hardlink to a file pointed by @src_filename
 *
 * @param     src_filename
 * @param     dst_filename
 * @returns   OK on success or else SYSERR
 */
int fs_link(char *src_filename, char *dst_filename);

/**
 * fs_unlink
 * Remove a hard link pointed by given name
 * If no hard links remain, file is deleted
 *
 * @param     filename
 * @returns   OK on success or else SYSERR
 */
int fs_unlink(char *filename);


/**
 * Filesystem functions
 */

/**
 * fs_mkfs
 * Create a file system on a device @dev
 * Write file system block and block bitmask to device @dev
 * Support for @num_inodes inodes
 *
 * @param     dev
 * @param     num_inodes
 * @returns   OK on success or else SYSERR
 */
int fs_mkfs(int dev, int num_inodes);

/**
 * fs_freefs
 * Remove a created file system on a device @dev
 *
 * @param     dev
 * @returns   OK on success or else SYSERR
 */
int fs_freefs(int dev);


/**
 * Filesystem internal functions
 */

/**
 * _fs_get_inode_by_num
 * Read in an inode by @inode_number and fill in the pointer @out
 *
 * @param     dev
 * @param     inode_number
 * @param     out
 * @returns   OK on success or else SYSERR
 */
int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out);

/**
 * _fs_put_inode_by_num
 * Write an inode by @inode_number by pointer @in
 *
 * @param     dev
 * @param     inode_number
 * @param     in
 * @returns   OK on success or else SYSERR
 */
int _fs_put_inode_by_num(int dev, int inode_number, inode_t *in);

/**
 * fs_setmaskbit
 * Set the block number @b in the mask
 *
 * @param     b
 * @returns   OK
 */
int fs_setmaskbit(int b);

/**
 * fs_getmaskbit
 * Get the block number @b in the mask
 *
 * @param     b
 * @returns   OK
 */
int fs_getmaskbit(int b);

/**
 * fs_clearmaskbit
 * Unset the block number @b in the mask
 *
 * @param     b
 * @returns   OK
 */
int fs_clearmaskbit(int b);


/**
 * Block Store functions
 */

/**
 * bs_mkdev
 * Create/make device and point it to dev0_blocks
 *
 * @param     dev device number
 * @param     blocksize
 * @param     numblocks
 * @returns   OK on success or else SYSERR
 */
int bs_mkdev(int dev, int blocksize, int numblocks);

/**
 * bs_freedev
 * Free device and all of its managed memory
 *
 * @param     dev device number
 * @returns   OK on success or else SYSERR
 */
int bs_freedev(int dev);

/**
 * bs_bread
 * Read @len bytes from @offset of @block of @bsdev into @buf
 *
 * @param     bsdev
 * @param     block
 * @param     offset
 * @param     buf
 * @param     len
 * @returns   OK on success or else SYSERR
 */
int bs_bread(int bsdev, int block, int offset, void *buf, int len);

/**
 * bs_bwrite
 * Write @len bytes from @buf to @offset of @block of @bsdev
 *
 * @param     bsdev
 * @param     block
 * @param     offset
 * @param     buf
 * @param     len
 * @returns   OK on success or else SYSERR
 */
int bs_bwrite(int bsdev, int block, int offset, void *buf, int len);

/**
 * Helper functions
 */
int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock);

/**
 * Debugging functions
 */

/**
 * fs_printfreemask
 * Print the entire mask
 */
void fs_printfreemask(void);

/**
 * fs_print_oft
 * Print content of the filetable
 */
void fs_print_oft(void);

/**
 * fs_print_inode
 * Print information about an inode @fd
 *
 * @param     fd
 */
void fs_print_inode(int fd);

/**
 * fs_print_fsd
 * Print content of the filesystem struct
 */
void fs_print_fsd(void);

/**
 * fs_print_dir
 * Print entries of the root directory
 */
void fs_print_dir(void);
void fs_print_dirs(directory_t*, int);
#endif /* FS_H */