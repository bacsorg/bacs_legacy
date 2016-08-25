#define _GNU_SOURCE
#include <assert.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int rm_funct(const char *fpath, const struct stat *sb, int typeflag,
             struct FTW *ftwbuf) {
  (void)sb;
  (void)ftwbuf;
  int ret;
  switch (typeflag) {
    case FTW_DP:
      ret = rmdir(fpath);
      break;
    case FTW_F:
      ret = unlink(fpath);
      break;
    default:
      fprintf(stderr, "Unknown file type, FIXME\n");
      return FTW_STOP;
  }
  if (ret) {
    perror(fpath);
    return FTW_STOP;
  }
  return FTW_CONTINUE;
}

int main(int argc, char **argv) {
  int res = 0, i;
  if (argc < 2) {
    fprintf(stderr, "Error: Argc(%d) != 2, run 'wipe [dirs...]'\n", argc);
    return 1;
  }
  for (i = 1; i < argc; ++i) {
    int ret = nftw(argv[i], rm_funct, 30, FTW_DEPTH | FTW_ACTIONRETVAL);
    int len = strlen(argv[i]);
    if (len && argv[i][len - 1] == '/')
      if (mkdir(argv[i], 0755)) {
        perror(argv[i]);
        res |= 4;
      }
    if (ret == FTW_STOP) {
      res |= 2;
    } else if (ret) {
      perror(argv[i]);
      res |= 1;
    }
  }
  return res;
}
