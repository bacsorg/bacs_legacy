#include "bacs2.h"

#include <csignal>
#include <cassert>

namespace
{
	sigset_t set;
	timespec timeout;
	bool wait_term()
	{
		int sig = sigtimedwait(&set, 0, &timeout);
		return sig==-1;
	}
	void siginit()
	{
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		sigaddset(&set, SIGTERM);
		sigaddset(&set, SIGHUP);
		assert(sigprocmask(SIG_BLOCK, &set, 0)==0);
		timeout.tv_sec = (cf_submits_delay+999)/1000;
		timeout.tv_nsec = 0;//TODO not really 0
	}
}

void check_thread_proc()
{
	bool need_announce = true;
	while (is_thread_running)
	{
		if (check_new_check_compiles())
		{
			need_announce = true;
			string sid = capture_new_checker_compilation( );
			if (sid != "")
			{
				compile_checker(sid);
			}
		}
		if (!check_new_submits())
		{
			if (need_announce) {
				log.add("Waiting for new submissions...");
				need_announce = false;
			}
			else
				log.add_working_notify();
			if (wait_term())
				return;
			continue;
		}
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

int main(int argc, char **argv)
{
	siginit();
	printf("BACS2 Server version %s\n", VERSION);
	//	string ts = "/usr/bin/c++";
	//char *arg_list [] = { "c++", "-x", "c++", "/home/zhent/bacs/b2/Temp/a.tmp", "-o/home/zhent/bacs/b2/Temp/a.tmp.o" };
	//	execv( ts.c_str( ), arg_list );

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

