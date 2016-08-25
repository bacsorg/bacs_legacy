#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define RUN_OK 0
#define RUN_FAILED 1
#define RUN_TIMEOUT 2
#define RUN_OUT_OF_MEMORY 3
#define RUN_ABNORMAL_EXIT 4
#define RUN_REALTIMEOUT 5
#define RUN_OUTPUT_LIMIT 6
#define RESULT_FILE_NAME "lim_run_results.txt"
#define MEMORY_LIMIT (1536 * 1024 * 1024)  // 1.5 GiB
#define OUTPUT_LIMIT (256 * 1024 * 1024)   // 256 MiB

int result = -1;
int exit_code = -1;
int memory_used = 0;
int time_used = 0;

string name_from_filename(const string &fn) {
  int k = (int)fn.find_last_of('/');
  if (k >= 0) return fn.substr(k + 1);
  return fn;
}

string dir_from_filename(const string &fn) {
  int k = (int)fn.find_last_of('/');
  if (k >= 0) return fn.substr(0, k + 1);
  return fn;
}

void writeResults() {
  FILE *f = fopen(RESULT_FILE_NAME, "w");
  fprintf(f, "%d %d %d %d\n", result, exit_code, memory_used, time_used);
  fclose(f);
}

bool file_exists(const char *fn) {
  struct stat stFileInfo;
  bool blnReturn;
  int intStat;

  // Attempt to get the file attributes
  intStat = stat(fn, &stFileInfo);
  if (intStat == 0) {
    // We were able to get the file attributes
    // so the file obviously exists.
    blnReturn = !S_ISDIR(stFileInfo.st_mode);
  } else {
    // We were not able to get the file attributes.
    // This may mean that we don't have permission to
    // access the folder which contains this file. If you
    // need to do that level of checking, lookup the
    // return values of stat which will give you
    // more details on why stat failed.
    blnReturn = false;
  }

  return (blnReturn);
}

int main(int argn, char **args) {
  if (file_exists(RESULT_FILE_NAME)) unlink(RESULT_FILE_NAME);

  if (argn < 7) {
    fprintf(stderr,
            "Error: You should run \"limit_run[0] cwd[1] in.txt[2] out.txt[3] "
            "TL[4] ML[5] redirect_stderr(yes/no)[6] RUN_FILE[7...]\"\n");
    return 1;
  }

  int memory_limit, time_limit;
  time_limit = atoi(args[4]);
  memory_limit = atoi(args[5]);
  string in_fn, out_fn;

  in_fn = args[2];
  out_fn = args[3];
  // argv
  char *executable = args[7];
  deque<string> argv_;
  for (size_t i = 8; args[i]; ++i) argv_.push_back(args[i]);
  char **argv = args + 7;
  if (name_from_filename(executable) == "java_run") {
    {
      stringstream buf;
      buf << "-Xss" << memory_limit;
      argv_.push_front(buf.str());
    }
    {
      stringstream buf;
      buf << "-Xmx" << memory_limit;
      argv_.push_front(buf.str());
    }
    argv_.push_front(executable);
    argv = new char *[argv_.size() + 1];
    for (size_t i = 0; i < argv_.size(); ++i) {
      argv[i] = new char[argv_[i].size() + 1];
      strcpy(argv[i], argv_[i].c_str());
    }
    argv[argv_.size()] = 0;
    memory_limit = 0;
  }
  int nid = fork();
  if (nid < 0) {
    fprintf(stderr, "Error: can't fork\n");
    return 1;
  }
  if (nid)  // parent
  {
    rusage lim;
    int status = 0;
    timespec nano_ts;
    int step = 200;
    nano_ts.tv_nsec = step * 1000000;
    nano_ts.tv_sec = 0;
    timespec st;
    clock_gettime(CLOCK_MONOTONIC, &st);
    while (1) {
      if (waitpid(nid, &status, WNOHANG)) break;

      nanosleep(&nano_ts, NULL);
      if (getrusage(RUSAGE_CHILDREN, &lim)) {
        time_used = 0;
      } else {
        time_used = lim.ru_utime.tv_sec * 1000 + lim.ru_utime.tv_usec / 1000;
      }

      timespec current_time;
      clock_gettime(CLOCK_MONOTONIC, &current_time);

      if (current_time.tv_sec >= st.tv_sec + max(time_limit / 100, 30) &&
          time_limit != 0) {
        kill(nid, 9);
        result = RUN_REALTIMEOUT;
        exit_code = 0;
        writeResults();
        return 0;
      }
    }

    if (getrusage(RUSAGE_CHILDREN, &lim)) {
      fprintf(stderr, "Error: can't getrusage\n");
      memory_used = 0;
      time_used = 0;
    } else {
      memory_used = lim.ru_maxrss * 1024;  // you need *BSD to use it
      time_used = lim.ru_utime.tv_sec * 1000 + lim.ru_utime.tv_usec / 1000;
    }

    exit_code = 0;
    WEXITSTATUS(status);
    if (WIFEXITED(status)) {
      result = RUN_OK;
      exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      switch (WTERMSIG(status)) {
        case SIGXCPU:
        case SIGALRM:
        case SIGVTALRM:
        case SIGPROF:
          if (time_used >= time_limit && time_limit != 0)
            result = RUN_TIMEOUT;
          else
            result = RUN_REALTIMEOUT;  // system time
          break;
        case SIGXFSZ:
          result = RUN_OUTPUT_LIMIT;
          break;
        default:
          result = RUN_ABNORMAL_EXIT;
      }
    } else {
      result = RUN_FAILED;
    }

    if (time_used >= time_limit && time_limit != 0) {
      result = RUN_TIMEOUT;
    } else if (memory_used >= memory_limit && memory_limit != 0) {
      result = RUN_OUT_OF_MEMORY;
    }
  } else  // child
  {
    {
      FILE *f = fopen(out_fn.c_str(), "w");
      fclose(f);
    }

    int fd_in, fd_out;
    fd_in = open(in_fn.c_str(), O_RDONLY);
    dup2(fd_in, STDIN_FILENO);
    fd_out = open(out_fn.c_str(), O_WRONLY);
    dup2(fd_out, STDOUT_FILENO);
    if (!strcmp(args[6], "yes")) dup2(fd_out, STDERR_FILENO);

    rlimit lim;
    if (memory_limit) {
      lim.rlim_cur = lim.rlim_max = MEMORY_LIMIT;
      setrlimit(RLIMIT_AS, &lim);
    }
    if (time_limit) {
      lim.rlim_cur = lim.rlim_max = time_limit / 1000 + 1;
      // hard limit is increased
      // to receive soft limit signal
      ++lim.rlim_max;
      setrlimit(RLIMIT_CPU, &lim);
    }
    {
      lim.rlim_cur = lim.rlim_max = OUTPUT_LIMIT;
      setrlimit(RLIMIT_FSIZE, &lim);
    }
    {
      lim.rlim_cur = lim.rlim_max = RLIM_INFINITY;
      setrlimit(RLIMIT_STACK, &lim);
    }

    if (chdir(args[1])) {
      perror("chdir");
      return 1;
    }
    if (execv(executable, argv)) {
      fprintf(stderr, "Error: can't execve\n");
      return 1;
    }
    return 2;
  }
  writeResults();

  return 0;
}
