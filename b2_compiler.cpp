#include "bacs2.h"

int check_thread;
//HANDLE event_wake;
bool is_thread_running;

int cf_submits_delay;
int cf_checker_timeout;
int cf_compiler_timeout;
int cf_job_query_period = JOB_QUERY_PERIOD;
int cf_max_idle_time;
//FILE *f_compile_log, *f_not_compiled;

string dbg_submit_id = "";
string nstr;

#define dir_exists file_exists

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

bool lock_table()
{
	string lock = db_qres0("select get_lock('olimp.submit_lock', 10)");
	return lock == "1";
}

void unlock_table()
{
	db_qres("select release_lock('olimp.submit_lock')");
}

bool check_new_submits()
{
	//STUB
	if (dbg_submit_id != "") return true;
	string q = db_qres0(format("select 1 from submit where result = %d limit 1", ST_PENDING));
	return q == "1";
}

string status_to_str(int status)
{
	switch (status)
	{
	case ST_SERVER_ERROR: return "Internal server error";
	case ST_ACCEPTED: return "Accepted";
	case ST_COMPILE_ERROR: return "Compilation error";
	case ST_MEMORY_LIMIT: return "Memory limit exceeded";
	case ST_TIME_LIMIT: return "Time limit exceeded";
	case ST_RUNTIME_ERROR: return "Runtime error";
	case ST_WRONG_ANSWER: return "Wrong answer";
	case ST_PRESENTATION_ERROR: return "Presentation error";
	case ST_SECURITY_VIOLATION: return "Security violation";
	case ST_INVALID_PROBLEM: return "Invalid problem id";
	case ST_PENDING: return "Pending check";
	case ST_RUNNING: return "Running";
	default: return "Unknown";
	}
}

bool test_submit(cstr sid)
{
	CSubmit s(sid);
	return s.compile_and_test();
}

string capture_new_submit()
{
	if (!lock_table()) {
		log.add_error(__FILE__, __LINE__, "Error: cannot lock table 'submit'!");
		return "";
	}
	string sid;
	if (dbg_submit_id != "")
	{
		sid = dbg_submit_id;
		dbg_submit_id = "";
	}
	else
	{
		sid = db_qres0(format("select submit_id from submit where result = %d limit 1", ST_PENDING));
	}
	if (sid != "")
		db_query(format("update submit set result = %d where submit_id = %s", ST_RUNNING, sid.c_str()));
	unlock_table();
	return sid;
}

int check_thread_proc( )
{
	bool need_announce = true;
	while (is_thread_running)
	{
		if (!check_new_submits()) {
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

bool init_config()
{
	if (!config.init(CONFIG_FILE_NAME)) {
		fprintf(stderr, "Fatal error: cannot read configuration! Config file: %s\n", CONFIG_FILE_NAME);
		return false;
	}
	cf_submits_delay = cfgi("general.check_submits_delay");
	cf_compiler_timeout = cfgi("general.compiler_timeout");
	cf_checker_timeout = cfgi("general.checker_timeout");
	cf_max_idle_time = cfgi("general.max_run_idle_time");
	return true;
}

string lang_str(cstr key, cstr lang, cstr src)
{
	string _lang = str_lowercase(lang);
	string s = cfg("lang." + _lang + "." + key);
	string dir = cfg("lang." + _lang + ".dir");
	string srcnoext = src.substr(0, src.find_last_of('.'));
	str_replace(s, "{src}", src);
	str_replace(s, "{dir}", dir);
	str_replace(s, "{src_noext}", srcnoext);
	return s;
}

int compile_source(cstr src_file, cstr lang, CTempFile *run_file, string &run_cmd, string &error_log)
{
	string pref = "lang." + str_lowercase(lang) + ".";
	int exit_code;
	string cmd = lang_str("compile", lang, src_file);
	if (run(cmd, &exit_code, true, "", error_log, cf_compiler_timeout ) != RUN_OK)
	{
		log.add("Error: cannot run compiler!", log.gen_data("Language", cfg(pref + "name"), "Command string", cmd));
		return COMPILE_FAILED;
	}
	string tmp_file = lang_str("tmpfile", lang, src_file);
	if (tmp_file != "") delete_file(tmp_file.c_str());
	string exe_file = lang_str("exefile", lang, src_file);
	if (!file_exists(exe_file))
	{
//		log.add("Error: no exe file!", log.gen_data("Language", cfg(pref + "name"), "EXENAME", exe_file));
		return COMPILE_ERROR;
	}
	if (run_file) run_file->assign(exe_file);
	run_cmd = lang_str("run", lang, src_file);
	return COMPILE_OK;
}

CSubmit::CSubmit(cstr _sid)
{
	sid = _sid;
	not_found = false;
	fatal_error = false;
	status = 0;
	info = "";
	max_time = 0; max_memory = 0;
	test_num_failed = -1;
	DBRow rr = db_qres(format("select problem_id, lang, solution, need_info from submit where submit_id = %s", sid.c_str()));
	if (rr.empty()) {
		not_found = true;
	}
	else
	{
		pid = rr[0]; lang = rr[1]; solution = rr[2]; need_info = (rr[3] == "1");
	}
}

bool CSubmit::compile_and_test()
{
	log.add(format("Testing submission [%s]", sid.c_str()));

	if (not_found) 
	{
		log.add("Error: submission not found!", log.gen_data("Submission ID", sid));
		return true;
	}
	if (compile()) {
		test();
		run_file.erase();
	}
	cleanup();
	store_result();

	log.add(format("Finished testing [%s]", sid.c_str()), log.gen_data("Result", status_to_str(status), "Failed on test", i2s(test_num_failed), "Time used", format("%.3lf", max_time), "Memory used", format("%.3lf", max_memory)));
	
	return !fatal_error;
}

bool CSubmit::compile()
{
	if (!security_check()) {
		status = ST_SECURITY_VIOLATION;
		return false;
	}
	if (cfg("lang." + str_lowercase(lang) + ".name") == "")
	{
		log.add("Error: unknown language!", log.gen_data("Language ID", lang));
		status = ST_SERVER_ERROR;
		return false;
	}
	if (!src_file.create(solution)) {
		log.add("Error: cannot create source file!");
		status = ST_SERVER_ERROR;
		return false;
	}
	int res = compile_source(src_file.name(), lang, &run_file, run_cmd, info);
	if (res != COMPILE_OK)
	{
		src_file.erase();
		if (res == COMPILE_ERROR) {
			status = ST_COMPILE_ERROR;
			need_info = true;
		}
		else status = ST_SERVER_ERROR;
		return false;
	}
	src_file.erase();
	return true;
}

bool CSubmit::test()
{
	CProblem problem;
	if (!problem.init(pid)) {
		log.add("Error initializing problem data!", log.gen_data("Problem ID", pid));
		status = ST_INVALID_PROBLEM;
		return false;
	}
	int st;
	int no_ml = cfgi( "lang." + str_lowercase( lang ) + ".no_memory_limit" );
	if ( no_ml )
		problem.set_no_memory_limit( );
	bool res = problem.run_tests(run_cmd, lang, st, max_time, max_memory, test_num_failed, need_info ? (&info) : NULL);
	if (!res) status = ST_SERVER_ERROR;
	else status = st;
	if (problem.is_fatal_error()) fatal_error = true;
	return res;
}

bool CSubmit::cleanup()
{
	string cs = lang_str("clean", lang, src_file.name());
	string err;
	if (cs == "") return true;
	if (run(cs, NULL, true, "", err, cf_compiler_timeout) != RUN_OK)
	{
		log.add("Error: error in cleanup process!", log.gen_data("Command string", cs, "Output", err));
		return false;
	}
	return true;
}

bool CSubmit::store_result()
{
	string s_info = need_info ? format(", info = '%s'", db.escape(info).c_str()) : "";
	return db_query(format("update submit set result = %li, test_num_failed = %s, max_time_used = %lf, max_memory_used = %lf%s where submit_id = %s",
		status, (test_num_failed < 0) ? "NULL" : i2s(test_num_failed).c_str(), max_time, max_memory, s_info.c_str(), sid.c_str()));
	return true;
}

bool CSubmit::security_check()
{
	char *buf = new char[solution.length() + 1];
	strcpy(buf, solution.c_str());
	char buf_info[256];
	bool res = SecurityCheck(buf, lang[0], buf_info, true);
	delete [] buf;
	if (!res && need_info)
	{
		info = buf_info;
	}
	return res;
}

CProblem::CProblem()
{
	fatal_error = false;
}

bool CProblem::init(cstr _id)
{
	id = _id;
	string dir = cfg("general.problem_archive_dir") + "/" + id + "/";
	string cf_fn = dir + "conf.txt";
	if (!cf.init(cf_fn)) return false;
	time_limit = s2d(cf.get("tl"), cfgd("general.default_time_limit"));
	memory_limit = s2d(cf.get("ml"), cfgd("general.default_memory_limit"));
	if (!init_checker()) return false;
	if (!init_tests()) return false;
	return true;
}

bool CProblem::run_tests(cstr run_cmd, cstr src_lang, int &result, double &max_time, double &max_memory, int &test_num_failed, string *info, bool acm, string & test_results)
{
	int i;
	max_time = 0;
	max_memory = 0;
	test_num_failed = -1;
	for (i = 0; i < (int)test.size(); ++i)
	{
		double time_used, memory_used;
		if (!run_test(test[i], run_cmd, src_lang, result, time_used, memory_used, info))
			return false;
		if (time_used > max_time) max_time = time_used;
		if (memory_used > max_memory) max_memory = memory_used;
		if (result != ST_ACCEPTED) {
			test_num_failed = test[i].id;
			break;
		}
	}
	return true;
}

bool tests_sort_func(cstr f1, cstr f2)
{
	int tn1 = CTest::parse_id(f1);
	int tn2 = CTest::parse_id(f2);
	return tn1 < tn2;
}

bool CProblem::init_tests()
{
	string dir = cfg("general.problem_archive_dir") + "/" + id + "/tests/";
	FileList f;
	if (!find_files(f, dir, ".in")) {
		log.add_error(__FILE__, __LINE__, "Error: cannot initialize tests!", log.gen_data("Problem ID", id));
		return false;
	}
	sort(f.begin(), f.end(), &tests_sort_func);
	int i;
	for (i = 0; i < (int)f.size(); ++i)
	{
		CTest t(f[i]);
		if (t.id != INVALID_ID) test.push_back(t);
	}
	is_tests_init = true;
	return true;
}

bool CProblem::init_checker()
{
	checker = cf.get("checker");
	string dir = cfg("general.problem_archive_dir") + "/" + id + "/checker/";
	string fn;
	if (checker == "") {
		fn = dir + "check.exe";
		if (file_exists(fn)) {
			checker = fn;
			return true;
		}
		fn = dir + "checker.exe";
		if (file_exists(fn)) {
			checker = fn;
			return true;
		}
	}
	if (checker == "default" || checker == "")
	{
		checker = cfg("general.default_checker");
		if (!file_exists(checker)) {
			log.add("Error: default checker does not exist!");
			return false;
		}
	}
	else
	{
		checker = dir + checker;
		if (!file_exists(checker)) {
			log.add("Error: checker does not exist!", log.gen_data("Checker file", checker));
			return false;
		}
	}
	return true;
}

bool CProblem::run_test(const CTest &tt, cstr run_cmd, cstr src_lang, int &result, double &time_used, double &memory_used, string *info)
{
	int t_used, m_used;
	string s_file_out;
	int ex;
	int limit_ml, limit_tl;
	limit_tl = (int)(time_limit * 1000);
	limit_ml = (int)(memory_limit * 1024 * 1024);
	int res = run(run_cmd, &ex, false, tt.file_in, s_file_out, limit_tl, limit_ml, t_used, m_used);
	time_used = (double)t_used / 1000;
	memory_used = (double)m_used / (1024 * 1024);
	bool ok = true;
	CTempFile s_out;
	s_out.assign(s_file_out);
	if (res == RUN_OK && ex == 0)
	{
		if (info) *info = s_out.read(cfgi("general.max_info_size"));
		string checker_cmd = format("%s %s %s %s", checker.c_str(), tt.file_in.c_str(), s_file_out.c_str(), tt.file_out.c_str());
//		log.add_error(__FILE__, __LINE__, "Start checker!", log.gen_data("Run command", checker_cmd));
		string checker_out;
		int res2 = run(checker_cmd, &ex, true, "", checker_out, cf_checker_timeout);
		if (res2 == RUN_FAILED || res2 == RUN_ABNORMAL_EXIT) {
			log.add_error(__FILE__, __LINE__, "Error: cannot run checker!", log.gen_data("Run command", checker_cmd));
			result = ST_SERVER_ERROR;
			ok = false;
		}
		else if (res2 == RUN_TIMEOUT) {
			log.add_error(__FILE__, __LINE__, "Error: checker timed out!", log.gen_data("Run command", checker_cmd, "Checker output", checker_out));
			result = ST_SERVER_ERROR;
			ok = false;
		}
		else
		{
			if (info && checker_out != "") *info += "\n === CHECKER OUTPUT ===\n" + checker_out;
			if (ex == CHECK_RES_OK) result = ST_ACCEPTED;
			else if (ex == CHECK_RES_WA || ex == CHECK_RES_WA_OLD ) result = ST_WRONG_ANSWER;
			else if (ex == CHECK_RES_PE) result = ST_PRESENTATION_ERROR;
			else {
				log.add_error(__FILE__, __LINE__, "Error: checker has returned unexpected result!", log.gen_data("Result code", i2s(ex)));
				result = ST_SERVER_ERROR;
				ok = false;
			}
		}
	}
	else
	{
		if (res == RUN_FAILED) {
			log.add_error(__FILE__, __LINE__, "Error: cannot run solution!", log.gen_data("Run command", run_cmd));
			result = ST_SERVER_ERROR;
			ok = false;
		}
		else if (res == RUN_TIMEOUT) result = ST_TIME_LIMIT;
		else if (res == RUN_OUT_OF_MEMORY) result = ST_MEMORY_LIMIT;
		else if (res == RUN_ABNORMAL_EXIT || (res == RUN_OK && ex != 0)) {
			result = ST_RUNTIME_ERROR;
		}
		else {
			log.add_error(__FILE__, __LINE__, "Error: unexpected run() result!", log.gen_data("Result", i2s(res))); 
			result = ST_SERVER_ERROR;
			ok = false;
		}
	}
	s_out.erase();
	return ok;
}

void CProblem::set_no_memory_limit( )
{
	memory_limit = 0.;
}

int CTest::parse_id(cstr fn)
{
	string nm = name_from_filename(fn);
	string ids, dummy;
	parse_str(nm, '.', ids, dummy);
	return s2i(ids, INVALID_ID);
}

CTest::CTest(cstr _file_in)
{
	file_in = _file_in;
	file_out = file_in.substr(0, file_in.find_last_of('.')) + ".out";
	id = parse_id(file_in);
}

const string CHECK_NAME = "check_new";
/*
void put_in_log( cstr dir, cstr info )
{
    fprintf( f_not_compiled, "%s\n", dir.c_str( ) );
    fprintf( f_compile_log, "/////////////////////////////////////////////////////\n" );
    fprintf( f_compile_log, "%s\n", dir.c_str( ) );
    fprintf( f_compile_log, "/////////////////////////////////////////////////////\n" );
    fprintf( f_compile_log, "%s", info.c_str( ) );
    fprintf( f_compile_log, "/////////////////////////////////////////////////////\n\n\n" );
} 
*/
string my_compile( cstr lang, cstr check_name, cstr check_dir )
{
	if (cfg("lang." + str_lowercase(lang) + ".name") == "")
	{
		log.add("Error: unknown language!", log.gen_data("Language ID", lang) );
		return "default";
	}
	CTempFile src_file, run_file;
	string run_cmd, info;
	src_file.assign( check_name );
	int res = compile_source( src_file.name(), lang, &run_file, run_cmd, info );
	if (res != COMPILE_OK)
	{
		log.add("Error: cannot compile file!");
		fprintf( stderr, "%s\n", info.c_str( ) );
//		put_in_log( check_dir, "Compile error:\n" + info );
		return "default";
	}
	string new_exe_name = check_dir + CHECK_NAME;
	if ( file_exists( new_exe_name ) )
		delete_file( new_exe_name );
	system( ( "ln " + run_file.name( ) + " " + new_exe_name ).c_str( ) );
	run_file.erase( );
	return CHECK_NAME;
}

string recompile_check_lang( cstr lang, cstr check_name, cstr check_dir )
{
	return my_compile( lang, check_name, check_dir );	
}

string recompile_checker( cstr check_dir )
{
	log.add( string( "Recompiling checker in: " ) + check_dir );
	int i;
	FileList fl_check;

	find_files( fl_check, check_dir, ".pas" );
	if ( !fl_check.empty( ) ) 
	{
		string check_name = fl_check[0];
		log.add( string( "Found checker: " ) + check_name );
		return recompile_check_lang( "P", check_name, check_dir );
	}

	find_files( fl_check, check_dir, ".dpr" );
	if ( !fl_check.empty( ) ) 
	{
		string check_name = fl_check[0];
		log.add( string( "Found checker: " ) + check_name );
		return recompile_check_lang( "P", check_name, check_dir );
	}

	find_files( fl_check, check_dir, ".c" );
	if ( !fl_check.empty( ) ) 
	{
		string check_name = fl_check[0];
		log.add( string( "Found checker: " ) + check_name );
		return recompile_check_lang( "C", check_name, check_dir );
	}

	find_files( fl_check, check_dir, ".cpp" );
	if ( !fl_check.empty( ) ) 
	{
		string check_name = fl_check[0];
		log.add( string( "Found checker: " ) + check_name );
		return recompile_check_lang( "+", check_name, check_dir );
	}
	
//	put_in_log( check_dir, "Checker (dpr,pas,c,cpp) not found.\n" );
	log.add( "Checker (*.dpr, *.pas, *.c, *.cpp ) not found" );
	return "default";
}

int main(int argc, char **argv)
{
	printf("BACS2 Checker compiler\n" );

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
/*	FileList fl_arch;
	if ( !find_dirs( fl_arch, cfg( "general.problem_archive_dir" ) + "/" ) )
	{
		log.add("Can't find problem archive dir content.\n");
		return 1;
	}
	
	f_compile_log = fopen( "compile.log", "w" );
	f_not_compiled = fopen( "not_compiled.log", "w" );

	int i;
	for ( i = 0; i < (int)fl_arch.size( ); ++ i )
*/	
	int res = 1;
	string problem_path = argv[1];
	if ( dir_exists( problem_path ) )
	{
	
		string problem_dir = problem_path + "/";
		string check_dir = problem_dir + "checker";
		log.add( string( "Looking for checker in dir: " ) + check_dir );
		char tmp_buf[1000];
		if ( dir_exists( check_dir ) )
		{
			string new_check_name = recompile_checker( check_dir + "/" );
			FILE * fo = fopen( ( problem_dir + "nconf.txt" ).c_str( ), "w" );
			FILE * f = fopen( ( problem_dir + "conf.txt" ).c_str( ), "r" );
			if ( f != NULL && fo != NULL ) 
			{
				while ( fgets( tmp_buf, sizeof( tmp_buf ) - 1, f ) )
				{
					string str = tmp_buf;
					trim( str );
					int pos = (int)str.find( "checker=" );
					if ( pos < 0 )
						fprintf( fo, "%s", tmp_buf );
				} 
				fprintf( fo, "checker=%s\n", new_check_name.c_str( ) );
			}
			if ( f ) fclose( f );
			if ( fo ) fclose( fo );
			delete_file( problem_dir + "conf.txt" );
			system( ( "mv " + problem_dir + "nconf.txt " + problem_dir + "conf.txt" ).c_str( ) );
		}
		res = 0;
	}
	else
	{
		res = 1;
	}

//	fclose( f_compile_log );
	log.add("Finished.");
	return res;
}
