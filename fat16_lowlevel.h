#ifndef _FAT16_LOWLEVEL_
#define _FAT16_LOWLEVEL_

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fat16.h"
#include "blkio.h"
#include "fat16_structs.h"

#define UNUSED __attribute__((unused))

typedef struct fuse_file_info fuse_file_info_t;

typedef struct {
  char *p;
  size_t size;
} dirbuf_t;

void fat16_init(void *userdata, struct fuse_conn_info *conn);
void fat16_getattr(fuse_req_t req, fuse_ino_t ino, fuse_file_info_t *fi UNUSED);
/**
   * Look up a directory entry by name and get its attributes.
   *
   * Valid replies:
   *   fuse_reply_entry
   *   fuse_reply_err
   *
   * @param req request handle
   * @param parent inode number of the parent directory
   * @param name the name to look up
   */
void fat16_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
void dirbuf_add(fuse_req_t req, dirbuf_t *b, const char *name, fuse_ino_t ino);
int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize, off_t off, size_t maxsize);
void fat16_opendir(fuse_req_t req, fuse_ino_t ino UNUSED, fuse_file_info_t *fi);
/**
   * Read directory
   *
   * Send a buffer filled using fuse_add_direntry(), with size not
   * exceeding the requested size.  Send an empty buffer on end of
   * stream.
   *
   * fi->fh will contain the value set by the opendir method, or
   * will be undefined if the opendir method didn't set any value.
   *
   * Returning a directory entry from readdir() does not affect
   * its lookup count.
   *
   * The function does not have to report the '.' and '..'
   * entries, but is allowed to do so.
   *
   * Valid replies:
   *   fuse_reply_buf
   *   fuse_reply_data
   *   fuse_reply_err
   *
   * @param req request handle
   * @param ino the inode number
   * @param size maximum number of bytes to send
   * @param off offset to continue reading the directory stream
   * @param fi file information
   */
void fat16_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info_t *fi UNUSED);
void fat16_releasedir(fuse_req_t req, fuse_ino_t ino UNUSED, fuse_file_info_t *fi UNUSED);
void fat16_open(fuse_req_t req, fuse_ino_t ino, fuse_file_info_t *fi);
void fat16_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info_t *fi UNUSED);
void fat16_release(fuse_req_t req, fuse_ino_t ino UNUSED, fuse_file_info_t *fi UNUSED);
void fat16_statfs(fuse_req_t req, fuse_ino_t ino UNUSED);

static struct fuse_lowlevel_ops fat16_oper = {
  .init = fat16_init,
  .lookup = fat16_lookup,
  .getattr = fat16_getattr,
  .opendir = fat16_opendir,
  .readdir = fat16_readdir,
  .releasedir = fat16_releasedir,
  .open = fat16_open,
  .read = fat16_read,
  .release = fat16_release,
  .statfs = fat16_statfs
};

#endif