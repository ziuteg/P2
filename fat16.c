
#include "fat16.h"

/* inodes -------------------------------- */

struct fat16_inode *fat16_inode(struct fat16_data *data, struct direntry dir) {
	struct fat16_inode *inode = malloc(sizeof(struct fat16_inode));

	inode->dir = dir;
	inode->ino = dir.deStartCluster + 10; // indeksowanie inodów indeksami klastrów

	return inode;
}

struct fat16_inode *fat16_find(struct fat16_data *data, fuse_ino_t ino) {
	struct fat16_inode *p;
	if (ino == 1) {
		return &data->root;
	}

	for (p = data->root.next; p != NULL; p = p->next) {
		if (p->ino == ino)
			return p;
	}
	return NULL;
}

void *fat16_insert(struct fat16_data *data, struct fat16_inode *inode) {
	struct fat16_inode *p;
	p = &data->root;
	while (p->next != NULL) {
		p = p->next;
	}
	p->next = inode;
	inode->next = NULL;
}

void fat16_free(struct fat16_inode *inode) {

}
/* --------------------------------------- */

struct fat16_data *fat16_data(fuse_req_t req) {
  return (struct fat16_data *) fuse_req_userdata(req);
}

time_t fat16_convert_time(uint16_t t, uint16_t d) {
	struct tm tm;
	tm.tm_sec = (t & 0x1f) * 2;
	tm.tm_min = (t >> 5) & 0x3f;
	tm.tm_hour = (t >> 11) & 0x1f;
	tm.tm_mday = (d & 0x1f);
	tm.tm_mon = ((d >> 5) & 0xf);
	tm.tm_year = ((d >> 9) & 0x7f) + 80;
	return mktime(&tm);
}

bool fat16_next_cluster(struct fat16_data *data, uint16_t *cluster) {
	struct bpb50 *bpb = &data->bootsector.bsBPB;
	size_t fat_offset = bpb->bpbResSectors;
	size_t fat_blocks = 1;

	printf("cluster try: %d\n", *cluster);

	fat_offset += ((*cluster) / bpb->bpbBytesPerSec);

	uint16_t *fat = malloc(sizeof(uint16_t) * bpb->bpbBytesPerSec);
	blk_read(data->fd, fat, fat_offset, fat_blocks);

	uint16_t next_cluster = fat[(*cluster) % bpb->bpbBytesPerSec];
	free(fat);

	if (0x3 <= next_cluster && next_cluster <= 0xffef) {
		printf("current cluster: %x\t next cluster: %x\n", *cluster, next_cluster);
		*cluster = next_cluster;
		return true;
	}
	else {
		printf("cluster end: %x\n", next_cluster);
		return false;
	}
}

int fat16_stat(struct fat16_inode *inode, struct stat *stbuf) {
  struct direntry dir = inode->dir;
  stbuf->st_atime = fat16_convert_time(0, dir.deADate);
  stbuf->st_mtime = fat16_convert_time(dir.deMTime, dir.deMDate);
  stbuf->st_ctime = fat16_convert_time(dir.deCTime, dir.deCDate);

  stbuf->st_ino = inode->ino;
  if (dir.deAttributes & ATTR_DIRECTORY) {
  	stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
  	stbuf->st_mode = S_IFREG | 0444;
  	stbuf->st_nlink = 1;
    stbuf->st_size = dir.deFileSize;
  }
  return 0;
}

char *fat16_convert_filename(struct direntry dir) {
	char *str = dir.deName;
	uint8_t attr = dir.deAttributes;

	size_t fn_len = 0;
	size_t ext_len = 0;
	size_t hidden = attr & ATTR_HIDDEN;

	for (int i = 0; i < 8 && str[i] != ' '; i++)
		fn_len++;

	if ((attr & ATTR_DIRECTORY) == 0)
		for (int i = 8; i < 11 && str[i] != ' '; i++)
			ext_len++;

	char *filename = calloc((fn_len + ext_len + hidden), sizeof(char));
	
	if (hidden)
		strcat(filename, ".");

	strncat(filename, str, fn_len);
	if (ext_len != 0) {
		strcat(filename, ".");
		strncat(filename, str + 8, ext_len);
	}
	return filename;
}