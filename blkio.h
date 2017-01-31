#ifndef _BLKIO_H_
#define _BLKIO_H_
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLKSIZE 512

int blk_open(const char *path);
void blk_close(int fd);
void blk_read(int fd, void *buf, size_t blkoff, size_t blkcnt);
void blk_write(int fd, const void *buf, size_t blkoff, size_t blkcnt);

#endif
