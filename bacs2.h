#ifndef _BACS2_H_
#define _XOPEN_SOURCE 600
#define _BACS2_H_

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <signal.h>
#include "bacs2_def.h"
#include "bacs2_log.h"
#include "bacs2_db.h"
#include "bacs2_tempfiles.h"
#include "bacs2_run.h"
#include "bacs2_legacy.h"

using namespace std;

extern int cf_submits_delay;
extern int cf_checker_timeout;
extern int cf_compiler_timeout;
extern int cf_compiler_memoryout;
extern int cf_job_query_period;
extern int cf_max_idle_time;
extern int cf_ping_period;
extern string cf_ping_uri;

extern string nstr;
extern string tests_for_check;

extern string dbg_submit_id;

class CSubmit;
class CProblem;

void console_display_help();
bool console_process_input(cstr s, int &exit_code);
bool lock_table();
void unlock_table();
bool check_new_submits();
bool test_submit(cstr sid);
bool start_check_thread();
void end_check_thread();
void wake_check_thread();
bool init_config();
bool test_submit(cstr sid);
string status_to_str(int status);
string lang_str(cstr key, cstr lang, cstr src);

int compile_source(cstr src_file, cstr lang, CTempFile *run_file, string &run_cmd, string &error_log);

bool tests_sort_func(cstr f1, cstr f2);

bool check_iofile_name(cstr fn);

bool check_new_check_compiles();
string capture_new_checker_compilation();
void compile_checker(const string &sid);
string capture_new_submit();

class CSubmit
{
private:
	string sid;
	string pid, lang, solution, test_results;
	bool acm;
	bool need_info;
	int status, test_num_failed;
	string info;
	bool fatal_error;
	bool not_found;
	CTempFile run_file;
	string run_cmd;
	double max_time, max_memory;
	CTempFile src_file;

	bool compile();
	bool test();
	bool cleanup();
	bool store_result();
	bool security_check();
public:
	CSubmit(cstr _sid);
	bool compile_and_test();
};

class CTest
{
public:
	int id;
	string file_in, file_out;
	static int parse_id(cstr fn);
	CTest(cstr _file_in);
};

class CProblem
{
	bool fatal_error;
	bool is_tests_init;
	bool is_checker_init;
	double time_limit, memory_limit;
	string id;
	string input_fn, output_fn;
	vector <CTest> test;
	string checker;
	bool init_checker();
	bool init_iofiles();
	bool init_tests();
	CCfgEngine cf;
public:
	void set_no_memory_limit();
	inline bool is_fatal_error() {return fatal_error;}
	CProblem();
	bool init(cstr _id);
	bool run_tests(cstr run_cmd, cstr src_lang, int &result, double &max_time, double &max_memory, int &test_num_failed, string *info, bool acm=true, string & test_results = nstr);
	bool run_test(const CTest &tt, cstr run_cmd, cstr src_lang, int &result, double &time_used, double &memory_used, string *info);
	inline bool dbg_init_tests() {return init_tests();}
};

extern int check_thread;
//extern HANDLE event_wake;
extern bool is_thread_running;

extern int cf_submits_delay;

#endif
