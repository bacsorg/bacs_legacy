#include "bacs2.h"

void console_display_help()
{
	printf("\nBACS2 Server version %s\n", VERSION);
	printf("Type 'q' to quit.\n");
}

bool console_process_input(cstr s, int &exit_code)
{
	string q, data;
	parse_str(s, ' ', q, data);
	if (q == "help" || q == "?") console_display_help();
	else if (q == "t") {
		dbg_submit_id = data;
		wake_check_thread();
	}
	else if (q == "q") return false;
	else printf("\nUnknown command: %s\n", s.c_str());
	return true;
}

int check_thread_proc( )
{
	bool need_announce = true;
	while (is_thread_running)
	{
		if (check_new_check_compiles( ) )
		{
			need_announce = true;
			string sid = capture_new_checker_compilation( );
			if ( sid != "" )
			{
				compile_checker( sid );
			}
		}
		if (!check_new_submits())
		{
			if (need_announce) {
				log.add("Waiting for new submissions...");
				need_announce = false;
			}
			else log.add_working_notify();
			//FIXME: No wake event; 
			sleep( (cf_submits_delay + 999 ) / 1000 );
			//WaitForSingleObject(event_wake, cf_submits_delay);
			continue;
		}
		need_announce = true;
		string sid = capture_new_submit();
		if (sid != "")
		{
			if (!test_submit(sid)) {
				log.add_error(__FILE__, __LINE__, "Terminating check thread due to fatal error!");
				return 1;
			}
		}
	}
	return 0;
}

bool start_check_thread()
{
	is_thread_running = true;
	int check_thread = fork( );  
	if ( check_thread )	
	{
		if ( check_thread < 0 )
			return false;
		return true;
		//parent
	}
	else
	{
		//child
		int res = check_thread_proc( );
		exit( res );
	}
/*	event_wake = CreateEvent(NULL, FALSE, FALSE, NULL);
	check_thread = CreateThread(NULL, 0, check_thread_proc, NULL, 0, NULL);
*/
}

void end_check_thread()
{
	is_thread_running = false;
	wake_check_thread();
	kill( check_thread, 9 );
//	CloseHandle( event_wake );
}

void wake_check_thread()
{
//	SetEvent(event_wake);
}


int main(int argc, char **argv)
{
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
	if (!start_check_thread())
	{
		log.add_error(__FILE__, __LINE__, "Fatal error: cannot start check thread!");
		return 1;
	}
	log.add("Started check thread.");

	int exit_code = 0;
	while(1)
	{
		char buf[256];
		gets( buf ); //its debug
		if (!console_process_input(buf, exit_code)) break;
	}
	log.add("Terminating check thread...");
	end_check_thread( );
	db.close();
	log.add("Finished.");
	return exit_code;
}
