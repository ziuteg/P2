
#include "fat16_lowlevel.h"


void fat16_init(void *userdata, struct fuse_conn_info *conn) {

  /* read the boot sector */
  struct fat16_data *data = (struct fat16_file *)userdata;
  blk_read(data->fd, &data->bootsector, 0, 1);
  struct bpb50 *bpb = &data->bootsector.bsBPB;
  //printf("bytes per sector: %d\nsectors per cluster: %d\nnumber of fats: %d\n", bpb->bpbBytesPerSec, bpb->bpbSecPerClust, bpb->bpbFATs);

  data->root_offset = bpb->bpbFATs * bpb->bpbFATsecs + bpb->bpbResSectors;
  /* find the root directory */
  size_t root_offset = bpb->bpbFATs * bpb->bpbFATsecs + bpb->bpbResSectors; // bpbResSectors usually contains + 1;
  size_t root_blocks = (bpb->bpbRootDirEnts * sizeof(struct direntry)) / bpb->bpbBytesPerSec; // ceil?
  
  //printf("root offset: %d\nFAT sectors: %d\nroot entries: %d\n", root_offset, bpb->bpbFATsecs, bpb->bpbRootDirEnts);
  
  data->root_dir = malloc(sizeof(struct direntry) * bpb->bpbRootDirEnts);
  blk_read(data->fd, data->root_dir, root_offset, root_blocks);


  /* debug: write root directory entries */
  for (int i = 0; i < bpb->bpbRootDirEnts; i++) {
    if (data->root_dir[i].deName[0] != 0) {
      printf("%d:\t%s\tcluster: %d\n", i, data->root_dir[i].deName, data->root_dir[i].deStartCluster);
    }
  }
  data->fat_offset = bpb->bpbResSectors;
  /* debug: write first FAT entries */
  size_t fat_offset = bpb->bpbResSectors;
  size_t fat_blocks = 1;
  uint16_t *fat = malloc(sizeof(uint16_t) * fat_blocks * bpb->bpbBytesPerSec);
  blk_read(data->fd, fat, fat_offset, fat_blocks);
  //printf("fat first entry: %x\nfat second entry: %x\n", fat[0], fat[1]);
  //for (int i = 0; i < bpb->bpbBytesPerSec; i+=4) {
  //  printf("%x %x %x %x\n", fat[i], fat[i+1], fat[i+2], fat[i+3]);
  //}
  free(fat);

  data->data_offset = data->root_offset + bpb->bpbRootDirEnts * sizeof(struct direntry) / bpb->bpbBytesPerSec;

  /* setup root inode */
  struct direntry dir = {.deAttributes = ATTR_DIRECTORY, .deStartCluster = 0};
  data->root.dir = dir;
  data->root.ino = 1;
}

void fat16_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, fuse_file_info_t *fi UNUSED) {

  struct fat16_data *data = fat16_data(req);
  dirbuf_t b;
  memset(&b, 0, sizeof(b));

  if (ino == 1) { /* reading root */
    struct bpb50 *bpb = &data->bootsector.bsBPB;
    struct direntry *root_dir = data->root_dir;

    for (int i = 0; i < bpb->bpbRootDirEnts; i++) {

      if (root_dir[i].deName[0] == 0) continue;
      if (root_dir[i].deAttributes & ATTR_VOLUME) continue;

      struct fat16_inode *inode = fat16_inode(data, root_dir[i]);
      if (fat16_find(data, inode->ino) == NULL)
        fat16_insert(data, inode);

      char *filename = fat16_convert_filename(root_dir[i]);
      dirbuf_add(req, &b, filename, inode->ino);
      free(filename);
    }

    reply_buf_limited(req, b.p, b.size, off, size);
    free(b.p);
  } else { /* reading subdirectories */

    struct fat16_inode *inode = fat16_find(data, ino);
    if (inode == NULL) {
      fuse_reply_err(req, ENOENT);
      return;
    }

    struct direntry dir = inode->dir;
    struct bpb50 *bpb = data->bootsector.bsBPB;

    uint16_t cluster = dir.deStartCluster;
    do {
      size_t cluster_size = bpb->bpbSecPerClust * bpb->bpbBytesPerSec;
      size_t dir_offset = data->data_offset + (cluster - 2) * bpb->bpbSecPerClust;
      size_t dir_blocks = bpb->bpbSecPerClust;
      size_t dir_cnt = cluster_size / sizeof(struct direntry);

      struct direntry *subdir = malloc(dir_cnt * sizeof(struct direntry));
      blk_read(data->fd, subdir, dir_offset, dir_blocks);

      /*
      printf("RAW TEXT\n");
      for (int i = 0; i < dir_cnt * sizeof(struct direntry); i++) {
        char x = *((char*)subdir + i);
        printf("%c", x); 
      }
      printf("\n");
      */

      for (int i = 0; i < dir_cnt; i++) {
        if (subdir[i].deName[0] == 0) continue;
        if (subdir[i].deAttributes & ATTR_VOLUME) continue;

        struct fat16_inode *sub_inode = fat16_inode(data, subdir[i]);
        if (fat16_find(data, sub_inode->ino) == NULL)
          fat16_insert(data, sub_inode);

        char *filename = fat16_convert_filename(subdir[i]);
        dirbuf_add(req, &b, filename, sub_inode->ino);
        free(filename);
      }
      free(subdir);
    } while (0);//(fat16_next_cluster(data, &cluster));

    reply_buf_limited(req, b.p, b.size, off, size);
    free(b.p);
  }
}

void fat16_getattr(fuse_req_t req, fuse_ino_t ino,
                          fuse_file_info_t *fi UNUSED) {
  struct stat stbuf;

  struct fat16_data *data = fat16_data(req);
  struct fat16_inode *inode = fat16_find(data, ino);

  if (fat16_stat(inode, &stbuf) == -1) {
    fuse_reply_err(req, ENOENT);
  } else {
    fuse_reply_attr(req, &stbuf, 1.0);
  }
}

 void fat16_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
  struct fuse_entry_param e;
  memset(&e, 0, sizeof(e));
  struct fat16_data *data = fat16_data(req);

  if (parent == 1) { /* lookup root directory */
    struct bpb50 *bpb = &data->bootsector.bsBPB;
    struct direntry *root_dir = data->root_dir;

    for (int i = 0; i < bpb->bpbRootDirEnts; i++) {
      char* filename = fat16_convert_filename(root_dir[i]);
        if (strcmp(name, filename) == 0) {

        struct fat16_inode *inode = fat16_inode(data, root_dir[i]);
        if (fat16_find(data, inode->ino) == NULL) {
          fuse_reply_err(req, ENOENT);
          return;
        }
          
        if (inode == NULL) {
          fuse_reply_err(req, ENOENT);
          return;
        }

        e.ino = inode->ino;
        e.attr_timeout = 1.0;
        e.entry_timeout = 1.0;
        fat16_stat(inode, &e.attr);
      }
      free(filename);
    }
  } else { /* lookup subdirectory */

    struct fat16_inode *par_inode = fat16_find(data, parent);
    if (par_inode == NULL) {
      fuse_reply_err(req, ENOENT);
      return;
    }

    struct direntry par_dir = par_inode->dir;
    struct bpb50 *bpb = &data->bootsector.bsBPB;

    uint16_t cluster = par_dir.deStartCluster;
    do {
      size_t cluster_size = bpb->bpbSecPerClust * bpb->bpbBytesPerSec;
      size_t dir_offset = data->data_offset + (cluster - 2) * bpb->bpbSecPerClust;
      size_t dir_blocks = bpb->bpbSecPerClust;
      size_t dir_cnt = cluster_size / sizeof(struct direntry);

      struct direntry *dir = malloc(dir_cnt * sizeof(struct direntry));
      blk_read(data->fd, dir, dir_offset, dir_blocks);

      /*
      printf("RAW TEXT\n");
      for (int i = 0; i < dir_cnt * sizeof(struct direntry); i++) {
        char x = *((char*)dir + i);
        printf("%c", x); 
      }
      printf("\n");
      */

      for (int i = 0; i < dir_cnt; i++) {
        char *filename = fat16_convert_filename(dir[i]);
        if (strcmp(name, filename) == 0) {
          struct fat16_inode *inode = fat16_inode(data, dir[i]);
          if (fat16_find(data, inode->ino) == NULL) {
            fuse_reply_err(req, ENOENT);
            return;
          }
          e.ino = inode->ino;
          e.attr_timeout = 1.0;
          e.entry_timeout = 1.0;
          fat16_stat(inode, &e.attr);
        }
        free(filename);
      }
      free(dir);
    } while (0);//(fat16_next_cluster(data, &cluster));
  }
  fuse_reply_entry(req, &e);
//  }
}

void dirbuf_add(fuse_req_t req, dirbuf_t *b, const char *name,
                       fuse_ino_t ino) {
  struct stat stbuf;
  size_t oldsize = b->size;
  b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
  b->p = (char *)realloc(b->p, b->size);
  memset(&stbuf, 0, sizeof(stbuf));
  stbuf.st_ino = ino;
  fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                    b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
                             off_t off, size_t maxsize) {
  if (off < (ssize_t)bufsize) {
    return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
  } else {
    return fuse_reply_buf(req, NULL, 0);
  }
}

 void fat16_opendir(fuse_req_t req, fuse_ino_t ino UNUSED,
                          fuse_file_info_t *fi) {
  fuse_reply_open(req, fi);
}

 void fat16_releasedir(fuse_req_t req, fuse_ino_t ino UNUSED,
                             fuse_file_info_t *fi UNUSED) {
  fuse_reply_err(req, ENOENT);
}

void fat16_open(fuse_req_t req, fuse_ino_t ino, fuse_file_info_t *fi) {
  struct fat16_data *data = fat16_data(req);
  struct fat16_inode *inode = fat16_find(data, ino);
  //printf("begin open\n");
  if (inode == NULL)
    fuse_reply_err(req, ENOENT);
  else if (inode->dir.deAttributes & ATTR_DIRECTORY)
    fuse_reply_err(req, EISDIR);
  else if ((fi->flags & 3) != O_RDONLY)
    fuse_reply_err(req, EACCES);
  else
    fuse_reply_open(req, fi);
  //printf("end open\n");
}

 void fat16_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                       fuse_file_info_t *fi UNUSED) {
  struct fat16_data *data = fat16_data(req);
  struct fat16_inode *inode = fat16_find(data, ino);
  
  if (inode == NULL) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  printf("begin read\t size:%d  deFileSize%d off:%d\n", size, inode->dir.deFileSize, off);
  //if (size != inode->dir.deFileSize) {
  //  fuse_reply_err(req, EINVAL);
  //  return;
  //}

  struct direntry dir = inode->dir;
  struct bpb50 *bpb = &data->bootsector.bsBPB;
  uint16_t cluster = dir.deStartCluster;
  size_t cluster_size = bpb->bpbSecPerClust * bpb->bpbBytesPerSec;

   for (int i = 0; i < off / cluster_size; i++)
    cluster = fat16_next_cluster;

  char *buf = malloc(sizeof(char) * size);
  char *tmp = malloc(sizeof(char) * cluster_size);
  size_t offset = 0;

  do {
    size_t buf_offset = data->data_offset + (cluster - 2) * bpb->bpbSecPerClust;
    size_t buf_blocks = bpb->bpbSecPerClust;

    blk_read(data->fd, tmp, buf_offset, buf_blocks);
    printf("test1 off %d cluster %d size %d\n", offset, cluster, size);
    if (offset + cluster_size > size) {
      memcpy(buf + offset, tmp, size - offset);
      //break;
    }
    else
      memcpy(buf + offset, tmp, cluster_size);
    printf("test2\n");
    offset += cluster_size;

  } while (0);(fat16_next_cluster(data, &cluster));

  printf("end read\t size:%d  deFileSize%d off:%d\n", size, inode->dir.deFileSize, off);
  

  fuse_reply_buf(req, buf, size);
  free(buf);
  free(tmp);
}

 void fat16_release(fuse_req_t req, fuse_ino_t ino UNUSED,
                          fuse_file_info_t *fi UNUSED) {
  fuse_reply_err(req, ENOENT);
}

 void fat16_statfs(fuse_req_t req, fuse_ino_t ino UNUSED) {
  struct statvfs statfs;
  memset(&statfs, 0, sizeof(statfs));
  fuse_reply_statfs(req, &statfs);
}