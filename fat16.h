
#ifndef _FAT16_H_
#define _FAT16_H_

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

 #include "blkio.h"
 #include "fat16_structs.h"

struct fat16_inode {
  struct fat16_inode *next;
  struct direntry dir;
  fuse_ino_t ino;
};

struct fat16_data {
  int fd;

  struct bootsector50 bootsector;
  struct bpb50 *bpb;
  struct direntry *root_dir;
  struct fat16_inode root;

  size_t fat_offset;
  size_t root_offset;
  size_t data_offset;

  char *path;
};

/* inode ---- */

struct fat16_inode *fat16_inode(struct fat16_data *data, struct direntry dir);

struct fat16_inode *fat16_find(struct fat16_data *data, fuse_ino_t ino);

void *fat16_insert(struct fat16_data *data, struct fat16_inode *inode);

void fat16_free();

/* ---------- */

bool fat16_next_cluster(struct fat16_data *data, uint16_t *cluster);

time_t fat16_convert_time(uint16_t t, uint16_t d);

struct fat16_data *fat16_data(fuse_req_t req);

char *fat16_convert_filename(struct direntry dir);

int fat16_stat(struct fat16_inode *inode, struct stat *stbuf);


#endif /* !_FAT16_H_ */
