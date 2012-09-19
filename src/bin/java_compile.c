#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int file_exists(const char *fn)
{
    struct stat stFileInfo;
    int blnReturn;
    int intStat;

    // Attempt to get the file attributes
    intStat = stat( fn, &stFileInfo );
    if(intStat == 0)
    {
        // We were able to get the file attributes
        // so the file obviously exists.
        blnReturn = !S_ISDIR( stFileInfo.st_mode );
    }
    else
    {
        // We were not able to get the file attributes.
        // This may mean that we don't have permission to
        // access the folder which contains this file. If you
        // need to do that level of checking, lookup the
        // return values of stat which will give you
        // more details on why stat failed.
        blnReturn = 0;
    }

    return blnReturn;
}

#define MAX_SIZE 1024
#define BUF_SIZE (MAX_SIZE+300)

void check_zero(int ret)
{
    if (ret)
    {
        perror("");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (argc!=3)
    {
        fprintf(stderr, "Error: Argc(%d) != 2, run 'java_compile JAVA_FILE NEW_NAME'\n", argc);
        return 1;
    }
    if (!file_exists(argv[1]))
    {
        fprintf(stderr, "Error: File not found, JAVA_FILE = %s'\n", argv[1]);
        return 1;
    }
    assert(strlen(argv[1])<MAX_SIZE);
    assert(strlen(argv[2])<MAX_SIZE);

    char dir[BUF_SIZE];
    sprintf(dir, "%s.dir", argv[1]);
    int ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    check_zero(ret);

    char file[BUF_SIZE];
    sprintf(file, "%s.dir/%s", argv[1], argv[2]);
    ret = link(argv[1], file);
    check_zero(ret);

    char buf[BUF_SIZE];
    sprintf(buf, "javac -g:none -classpath %s.dir %s.dir/%s", argv[1], argv[1], argv[2]);
    ret = system(buf);
    if (!WIFEXITED(ret) || WEXITSTATUS(ret)!=0)
    {
        fprintf(stderr, "Error while executing javac\n");
        return 1;
    }

    ret = unlink(file);
    check_zero(ret);

    return 0;
}
