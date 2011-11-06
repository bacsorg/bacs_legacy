#include <unistd.h>

int main(int argc, char **argv)
{
	chdir("Test");
	execv(argv[1], argv+1);
}

