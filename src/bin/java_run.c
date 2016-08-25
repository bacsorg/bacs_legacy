#include <unistd.h>

int main(int argc, char **argv) {
  chdir("Test");
  return execvp("java", argv);
}
