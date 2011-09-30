#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>

#define MAX_SIZE 1024
#define BUF_SIZE (MAX_SIZE+300)

int rm_funct(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	int ret;
	switch (typeflag)
	{
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
	if (ret)
	{
		perror(fpath);
		return FTW_STOP;
	}
	return FTW_CONTINUE;
}

int main(int argc, char **argv)
{
	if (argc!=2)
	{
		fprintf(stderr, "Error: Argc(%d) != 2, run 'java_clean JAVA_FILE'\n", argc);
		return 1;
	}
	assert(strlen(argv[1])<MAX_SIZE);
	char dir[BUF_SIZE];
	sprintf(dir, "%s.dir", argv[1]);

	int ret = nftw(dir, rm_funct, 30, FTW_DEPTH|FTW_ACTIONRETVAL);
	if (ret==FTW_STOP)
	{
		return 2;
	}
	else
	{
		perror(dir);
		return 1;
	}
	return 0;
}

