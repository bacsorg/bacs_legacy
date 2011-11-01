#include <cstdio>
#include <cstdlib>
#include <cstring>
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

using namespace std;

#define RUN_OK 0
#define RUN_FAILED 1
#define RUN_TIMEOUT 2
#define RUN_OUT_OF_MEMORY 3
#define RUN_ABNORMAL_EXIT 4
#define RUN_REALTIMEOUT 5
#define RESULT_FILE_NAME "lim_run_results.txt"
#define MAX_TIME (60*10)
#define ADD_TO_CHECK 8000000

int result = -1;
int exit_code = -1;
int memory_used = 0;
int time_used = 0;

string name_from_filename(const string &fn)
{
	int k = (int)fn.find_last_of('/');
	if (k >= 0) return fn.substr(k + 1);
	return fn;
}

string dir_from_filename(const string &fn)
{
	int k = (int)fn.find_last_of('/');
	if (k >= 0) return fn.substr(0, k + 1);
	return fn;
}

void writeResults()
{
	FILE *f = fopen(RESULT_FILE_NAME, "w");
	fprintf(f, "%d %d %d %d\n", result, exit_code, memory_used, time_used);
	fclose(f);
}

bool file_exists(const char *fn)
{
  struct stat stFileInfo;
  bool blnReturn;
  int intStat;

  // Attempt to get the file attributes
  intStat = stat(fn, &stFileInfo);
  if(intStat == 0) {
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

int main(int argn, char ** args)
{
	chdir(dir_from_filename(args[0]).c_str());

	if (file_exists(RESULT_FILE_NAME))
		unlink(RESULT_FILE_NAME);

	if (argn < 7)
	{
		fprintf(stderr, "Error: You should run \"limit_run[0] in.txt[1] out.txt[2] TL[3] ML[4] redirect_stderr(yes/no)[5] RUN_FILE[6...]\"\n");
		return 1;
	}

	int memory_limit, time_limit;
	time_limit = atoi(args[3]);
	memory_limit = atoi(args[4]);
	string in_fn, out_fn;

	in_fn = args[1];
	out_fn = args[2];

	int nid = fork();
	if (nid < 0)
	{
		fprintf(stderr, "Error: can't fork\n");
		return 1;
	}
	if (nid) //parent
	{

		rusage lim;
		int status = 0;
    	timespec nano_ts;
    	int waited = 0;
    	int step = 200;
   		nano_ts.tv_nsec = step * 1000000;
		nano_ts.tv_sec = 0;
		timespec st;
		clock_gettime(CLOCK_MONOTONIC, &st);
		while (1)
		{
			if (waitpid(nid, &status, WNOHANG))
				break;

			nanosleep(&nano_ts, NULL);
			if (getrusage(RUSAGE_CHILDREN, &lim))
			{
				time_used = 0;
			}
			else
			{
				time_used = lim.ru_utime.tv_sec * 1000 + lim.ru_utime.tv_usec / 1000;
			}

			timespec current_time;
			clock_gettime(CLOCK_MONOTONIC, &current_time);

			if (current_time.tv_sec >= st.tv_sec+MAX_TIME && time_limit != 0)
			{
				kill(nid, 9);
				result = RUN_REALTIMEOUT;
				exit_code = 0;
				writeResults();
				return 0;
			}
		}

		if (getrusage(RUSAGE_CHILDREN, &lim))
		{
			fprintf(stderr, "Error: can't getrusage\n");
			memory_used = 0;
			time_used = 0;
		}
		else
		{
			memory_used = lim.ru_maxrss * 1024; //you need *BSD to use it
			time_used = lim.ru_utime.tv_sec * 1000 + lim.ru_utime.tv_usec / 1000;
		}

		exit_code = 0;
		WEXITSTATUS(status);
		if (WIFEXITED(status))
		{
			result = RUN_OK;
			exit_code = WEXITSTATUS(status);
		}
		else
		if (WIFSIGNALED(status))
		{
			int term_code = WTERMSIG(status);
/*			if (term_code == SIGKILL)
			{
				result = RUN_OK;
			}
			else
*/			if (term_code == SIGXCPU || term_code == SIGALRM ||
				term_code == SIGVTALRM || term_code == SIGPROF)
			{
				result = RUN_TIMEOUT;
			}
			else
			{
				result = RUN_ABNORMAL_EXIT;
			}
		}
		else
		{
			result = RUN_FAILED;
		}

		if (time_used >= time_limit && time_limit != 0)
		{
			result = RUN_TIMEOUT;
		}
		else
		if (memory_used >= memory_limit && memory_limit != 0)
		{
			result = RUN_OUT_OF_MEMORY;
		}
	}
	else //child
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
		if (!strcmp(args[5], "yes"))
			dup2(fd_out, STDERR_FILENO);

		rlimit lim;
		if (memory_limit)
		{
			lim.rlim_cur = lim.rlim_max = memory_limit + ADD_TO_CHECK;
			setrlimit(RLIMIT_AS, &lim);
		}
		if (time_limit)
		{
			lim.rlim_cur = lim.rlim_max = time_limit/1000+1;
			setrlimit(RLIMIT_CPU, &lim);
		}

		if (execv(args[6], args + 6))
		{
			fprintf(stderr, "Error: can't execve\n");
			return 1;
		}
		return 2;
	}
	writeResults();

	return 0;
}
