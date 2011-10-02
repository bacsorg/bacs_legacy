#include "bacs2.h"

#include <clocale>
#include <csignal>
#include <cassert>

namespace
{
	sigset_t set;
	timespec timeout;
	bool wait_term(long sec=0, long nsec=1000*1000*100)
	{
		timeout.tv_sec = sec;
		timeout.tv_nsec = nsec;
		int sig = sigtimedwait(&set, 0, &timeout);
		return sig!=-1;
	}
	bool short_wait_term()
	{
		return wait_term(0, 1000*1000);
	}
	bool siginit()
	{
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		sigaddset(&set, SIGTERM);
		sigaddset(&set, SIGHUP);
		int ret = sigprocmask(SIG_BLOCK, &set, 0);
		if (ret)
		{
			perror("");
			return false;
		}
		return true;
	}
}

void check_thread_proc()
{
	bool need_announce = true;
	for (;;)
	{
		if (short_wait_term())
			return;
		if (check_new_check_compiles())
		{
			need_announce = true;
			string sid = capture_new_checker_compilation();
			if (sid != "")
			{
				compile_checker(sid);
			}
		}
		if (short_wait_term())
			return;
		if (!check_new_submits())
		{
			if (need_announce) {
				log.add("Waiting for new submissions...");
				need_announce = false;
			}
			else
				log.add_working_notify();
			timespec cur, end;
			clock_gettime(CLOCK_MONOTONIC, &cur);
			end.tv_nsec = (cur.tv_nsec+(cf_submits_delay%1000)*1000000)%(1000*1000*1000);
			end.tv_sec = cur.tv_sec+cf_submits_delay/1000+end.tv_nsec/(1000*1000*1000);
			for (;;)
			{
				if (wait_term())
					return;
				clock_gettime(CLOCK_MONOTONIC, &cur);
				if (cur.tv_sec>end.tv_sec || (cur.tv_sec==end.tv_sec && cur.tv_nsec>end.tv_nsec))
					break;
			}
			continue;
		}
		if (short_wait_term())
			return;
		need_announce = true;
		string sid = capture_new_submit();
		if (sid != "")
		{
			if (!test_submit(sid))
			{
				log.add_error(__FILE__, __LINE__, "Terminating check thread due to fatal error!");
				return;
			}
		}
	}
	return;
}

void init_env()
{
	setlocale(LC_ALL, "C");
	setenv("LANG", "C", 1);
}

int main(int argc, char **argv)
{
	if (!siginit())
		return 2;
	init_env();
	printf("BACS2 Server version %s\n", VERSION);

	chdir(dir_from_filename(argv[0]));

	if (!init_config())
	{
		return 1;
	}

	if (!log.init())
	{
		fprintf(stderr, "Error: cannot initialize log system!\n");
		return 1;
	}
	log.add("Starting new session...");

	if (!db.connect()) {
		log.add_error(__FILE__, __LINE__, "Fatal error: cannot connect to database!");
		return 1;
	}
	log.add("Connected to database.");


	check_thread_proc();

	int exit_code = 0;
	db.close();
	log.add("Finished.");
	return exit_code;
}

