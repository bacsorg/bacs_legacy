#include "bacs2.h"

int check_thread;
//HANDLE event_wake;
bool is_thread_running;

int cf_submits_delay;
int cf_checker_timeout;
int cf_compiler_timeout;
int cf_job_query_period = JOB_QUERY_PERIOD;
int cf_max_idle_time;

string dbg_submit_id = "";

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

bool check_new_check_compiles( )
{
	string q = db_qres0("select 1 from compile_checkers where state = 0 limit 1");
	return q == "1";
}

string capture_new_checker_compilation( )
{
	if (!lock_table()) {
		log.add_error(__FILE__, __LINE__, "Error: cannot lock table 'submit'!");
		return "";
	}
	string sid = db_qres0("select check_id from compile_checkers where state = 0 limit 1");
	if (sid != "")
		db_query(format("update compile_checkers set state = %d where check_id = %s", STATE_COMPILING, sid.c_str()));
	unlock_table();
	return sid;
	
}

void compile_checker( const string &sid )
{
	string rr = db_qres0(format("select problem_id from compile_checkers where check_id = %s", sid.c_str()));
	if (rr == "") {
		return;
	}
	else
	{
		log.add( "Compiling checker for problem: " + rr );
		int exit_code;
		string error_log;
		string cmd = cfg("general.b2_compiler") + " " + cfg("general.problem_archive_dir") + "/" + rr;
		if (run(cmd, &exit_code, true, "", error_log, cf_compiler_timeout ) != RUN_OK)
		{
			log.add( ("Error: cannot compile checker!" + cmd + "\n" + error_log ) );
		}			
	}
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
	if (!init_iofiles()) return false;
	if (!init_checker()) return false;
	if (!init_tests()) return false;
	return true;
}

bool CProblem::run_tests(cstr run_cmd, cstr src_lang, int &result, double &max_time, double &max_memory, int &test_num_failed, string *info)
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

bool check_iofile_name( cstr fn )
{
	int n = (int)fn.size();
	int dot_cnt = 0;
	for ( int i = 0; i < n; ++ i )
	{
		if ( fn[i] >= '0' && fn[i] <= '9' )
			continue;
		if ( fn[i] >= 'a' && fn[i] <= 'z' )
			continue;
		if ( fn[i] >= 'A' && fn[i] <= 'Z' )
			continue;
		if ( fn[i] == '_' )
			continue;
		if ( fn[i] == '.' )
		{
			++ dot_cnt;
			if ( dot_cnt == 2 )
				return false;
		}
		else
			return false;
	}
	return true;
}

bool CProblem::init_iofiles()
{
	input_fn = cf.get("input");
	if ( input_fn == "" )
		input_fn = "STDIN";
	if ( !check_iofile_name( input_fn ) )
	{
		log.add("Error: input file is incorrect!", log.gen_data("Input file", input_fn ));
		return false;
	}
		
	output_fn = cf.get("output");
	if ( output_fn == "" )
		output_fn = "STDOUT";
	if ( !check_iofile_name( output_fn ) )
	{
		log.add("Error: output file is incorrect!", log.gen_data("Output file", output_fn ));
		return false;
	}
	
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
	int res = run_fio(run_cmd, &ex, tt.file_in, s_file_out, limit_tl, limit_ml, t_used, m_used, input_fn, output_fn );
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
