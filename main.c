
#define FUSE_USE_VERSION 30

#include <config.h>

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

/*
 * BIOS Parameter Block (BPB) for DOS 3.3
 */
struct parameter_block {
	uint16_t	bytes_per_sector;	 /* bytes per sector */
	uint8_t		sectors_per_cluster; /* sectors per cluster */
    uint16_t	reserved_sectors;  	 /* number of reserved sectors */
    uint8_t		fats;       		 /* number of FATs */
    uint16_t	root_entries;		 /* number of root directory entries */
    uint16_t	sectors;			 /* total number of sectors */
    uint8_t		media_descriptor;    /* media descriptor */
    uint16_t	sectors_per_fat;	 /* number of sectors per FAT */
    uint16_t	sectors_per_track; 	 /* sectors per track */
    uint16_t	heads;       		 /* number of heads */
    uint16_t	hidden_sectors;		 /* number of hidden sectors */
};

/*
 * FAT directory entry
 */
struct directory_entry {
	char		filename[8];
	char		filename_extension[3];
	uint8_t		file_attributes;
	uint16_t	time_modified; /* time created or last updated */
	uint16_t	date_modified; /* date created or last modified */
	uint16_t	starting_cluster;
	uint32_t	file_size;
}

static void fat16_open(fuse_req_t req, fuse_ino_t ino,
struct fuse_file_info *fi) {
}

static void fat16_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
struct fuse_file_info *fi) {

}

static void fat16_release(fuse_req_t req, struct fuse_file_info *fi) {

}

static void fat16_getattr(fuse_req_t req, fuse_ino_t ino,
struct fuse_file_info *fi) {

}

static void fat16_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {

}

static void fat16_opendir(fuse_req_t req, fuse_ino_t ino,
struct fuse_file_info *fi) {

}

static void fat16_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
struct fuse_file_info *fi) {

}

static void fat16_releasedir(fuse_req_t req, fuse_ino_t ino,
struct fuse_file_info *fi) {

}

static void fat16_statfs(fuse_req_t req, fuse_ino_t ino) {

}

static struct fuse_lowlevel_ops fat16_oper = {
	.getattr 	= fat16_getattr,
	.lookup 	= fat16_lookup,
	.open 		= fat16_open,
	.opendir 	= fat16_opendir,
	.read 		= fat16_read,
	.readdir 	= fat16_readdir,
	.release 	= fat16_release,
	.releasedir = fat16_releasedir,
	.statfs 	= fat16_statfs,
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_session *se;
	struct fuse_cmdline_opts opts;
	int ret = -1;

	if (fuse_parse_cmdline(&args, &opts) != 0)
		return 1;
	if (opts.show_help) {
		printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
		fuse_cmdline_help();
		fuse_lowlevel_help();
		ret = 0;
		goto err_out1;
	} else if (opts.show_version) {
		printf("FUSE library version %s\n", fuse_pkgversion());
		fuse_lowlevel_version();
		ret = 0;
		goto err_out1;
	}

	se = fuse_session_new(&args, &hello_ll_oper,
			      sizeof(hello_ll_oper), NULL);
	if (se == NULL)
	    goto err_out1;

	if (fuse_set_signal_handlers(se) != 0)
	    goto err_out2;

	if (fuse_session_mount(se, opts.mountpoint) != 0)
	    goto err_out3;

	fuse_daemonize(opts.foreground);

	/* Block until ctrl+c or fusermount -u */
	if (opts.singlethread)
		ret = fuse_session_loop(se);
	else
		ret = fuse_session_loop_mt(se, opts.clone_fd);

	fuse_session_unmount(se);
err_out3:
	fuse_remove_signal_handlers(se);
err_out2:
	fuse_session_destroy(se);
err_out1:
	free(opts.mountpoint);
	fuse_opt_free_args(&args);

	return ret ? 1 : 0;
}