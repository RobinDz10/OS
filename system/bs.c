/**
 * This file implements the memory "block store" for use by the in-memory filesystem
 */
#include <xinu.h>

extern int dev0_numblocks;
extern int dev0_blocksize;
extern char *dev0_blocks;
int dev0 = 0;

#if FS
#include <fs.h>

int bs_mkdev(int dev, int blocksize, int numblocks) {

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (blocksize != 0) {
    dev0_blocksize = blocksize;
  } else {
    dev0_blocksize = MDEV_BLOCK_SIZE;
  }

  if (numblocks != 0) {
    dev0_numblocks =  numblocks;
  } else {
    dev0_numblocks =  MDEV_NUM_BLOCKS;
  }

  if ((dev0_blocks = getmem(dev0_numblocks * dev0_blocksize)) == (void *) SYSERR) {
    errormsg("mkbsdev memgetfailed\n");
    return SYSERR;
  }

  return OK;

}

int bs_freedev(int dev) {

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (freemem(dev0_blocks, dev0_numblocks * dev0_blocksize) == SYSERR) {
    errormsg("bs_freedev freemem failed\n");
    return SYSERR;
  }

  return OK;

}

int bs_bread(int dev, int block, int offset, void *buf, int len) {

  char *bbase;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (offset < 0 || offset >= dev0_blocksize) {
    errormsg("Bad offset: %d\n", offset);
    return SYSERR;
  }

  bbase = &dev0_blocks[block * dev0_blocksize];

  memcpy(buf, (bbase + offset), len);

  return OK;

}

int bs_bwrite(int dev, int block, int offset, void *buf, int len) {

  char *bbase;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (offset < 0 || offset >= dev0_blocksize) {
    errormsg("Bad offset: %d\n", offset);
    return SYSERR;
  }
  if (offset + len > dev0_blocksize) {
    errormsg("Bad length: %d\n", len);
    return SYSERR;
  }

  bbase = &dev0_blocks[block * dev0_blocksize];

  memcpy((bbase + offset), buf, len);

  return OK;

}

#endif /* FS */