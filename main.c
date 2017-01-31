#define FUSE_USE_VERSION 30

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fat16_lowlevel.h"
#include "fat16.h"
#include "blkio.h"
#include "fat16_structs.h"

int main(int argc, char *argv[]) {

  struct fat16_data data = {.path = argv[2]};
  data.fd = blk_open(data.path);

  /* init filesystem */
  struct fuse_args args = FUSE_ARGS_INIT(argc-1, argv);
  struct fuse_session *se;
  struct fuse_cmdline_opts opts;
  int ret = -1;

  if (fuse_parse_cmdline(&args, &opts) != 0) return 1;

  se = fuse_session_new(&args, &fat16_oper, sizeof(fat16_oper), &data);
  if (se != NULL) {
    if (fuse_set_signal_handlers(se) != -1) {
      fuse_session_mount(se, opts.mountpoint);
      ret = fuse_session_loop(se);
      fuse_remove_signal_handlers(se);
    }
    fuse_session_unmount(se);
  }
  fuse_session_destroy(se);

  free(opts.mountpoint);
  fuse_opt_free_args(&args);

  free(data.root_dir);

  return ret ? 1 : 0;
}
