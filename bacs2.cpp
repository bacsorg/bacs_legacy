#include "bacs2.h"

enum
{
	order_ascending,
	order_descending,
	order_random
};

int cf_submits_delay;
int cf_checker_timeout;
int cf_compiler_timeout;
int cf_compiler_memoryout;
int cf_job_query_period = JOB_QUERY_PERIOD;
int cf_max_idle_time;

int cf_ping_period;
string cf_ping_uri;

int cf_order;
int cf_compile_checkers;
int cf_check_solutions;
string cf_langs_config;

string nstr;
string tests_for_check;

string dbg_submit_id = "";

std::auto_ptr<bunsan::pm::compatibility::repository> repository;

bool check_new_submits()
{
	//STUB
	if (dbg_submit_id != "") return true;
	string q = db_qres0(format("select 1 from submit where result = %d limit 1", ST_PENDING));
	return q == "1";
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
		int ord = cf_order;
		if (ord==order_random)
			ord = rand()&1?order_ascending:order_descending;
		sid = db_qres0(format(
			"select submit_id from submit where result = %d order by submit_id %s limit 1",
			ST_PENDING,
			ord==order_ascending?"asc":"desc"));
	}
	if (sid != "")
		db_query(format("update submit set result = %d where submit_id = %s", ST_RUNNING, sid.c_str()));
	unlock_table();
	return sid;
}

bool check_new_check_compiles()
{
	string q = db_qres0("select 1 from compile_checkers where state = 0 limit 1");
	return q == "1";
}

string capture_new_checker_compilation()
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

void compile_checker(const string &sid)
{
	string rr = db_qres0(format("select problem_id from compile_checkers where check_id = %s", sid.c_str()));
	if (rr == "") {
		return;
	}
	else
	{
		log.add("Compiling checker for problem: " + rr);
		int exit_code;
		string error_log;
		string cmd = cfg("general.b2_compiler") + " " + cfg("general.problem_archive_dir") + "/" + rr;
		if (run(cmd, &exit_code, true, "", error_log, cf_compiler_timeout) != RUN_OK)
		{
			log.add(("Error: cannot compile checker!" + cmd + "\n" + error_log));
		}
		db_query(format("update compile_checkers set state=1 where check_id = %s", sid.c_str()));
	}
}

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

bool init_config()
{
	if (!config.init(CONFIG_FILE_NAME))
	{
		fprintf(stderr, "Fatal error: cannot read configuration! Config file: %s\n", CONFIG_FILE_NAME);
		return false;
	}
	cf_submits_delay = cfgi("general.check_submits_delay");
	cf_compiler_timeout = cfgi("general.compiler_timeout");
	cf_compiler_memoryout = cfgi("general.compiler_memoryout");
	cf_checker_timeout = cfgi("general.checker_timeout");
	cf_max_idle_time = cfgi("general.max_run_idle_time");
	cf_ping_period = cfgi("general.ping_period");
	cf_ping_uri = cfg("general.ping_uri");
	{
		string ord = cfg("general.order");
		if (ord=="asc")
			cf_order = order_ascending;
		else if (ord=="desc")
			cf_order = order_descending;
		else if (ord=="rand")
			cf_order = order_random;
		else
			return false;
	}
	cf_compile_checkers = cfgi("general.compile_checkers");
	cf_check_solutions = cfgi("general.check_solutions");
	cf_langs_config = cfg("langs.config");
	boost::property_tree::ptree repo_config;
	boost::property_tree::read_info(cfg("general.bunsan_repository_config"), repo_config);
	repository.reset(new bunsan::pm::compatibility::repository(repo_config));
	if (!langs_config.init(cf_langs_config))
	{
		fprintf(stderr, "Fatal error: cannot read langs configuration! Config file: %s\n", cf_langs_config.c_str());
		return false;
	}
	return true;
}

int compile_source(cstr src_file, cstr lang, CTempFile *run_file, string &run_cmd, string &error_log)
{
	string pref = str_lowercase(lang) + ".";
	int exit_code;
	string cmd = lang_str("compile", lang, src_file);
	if (run(cmd, &exit_code, true, "", error_log, cf_compiler_timeout, cf_compiler_memoryout) != RUN_OK)
	{
		log.add("Error: cannot run compiler!", log.gen_data("Language", lcfg(pref + "name"), "Command string", cmd));
		return COMPILE_FAILED;
	}
	string tmp_file = lang_str("tmpfile", lang, src_file);
	if (tmp_file != "") delete_file(tmp_file.c_str());
	string exe_file = lang_str("exefile", lang, src_file);
	if (!file_exists(exe_file))
	{
//		log.add("Error: no exe file!", log.gen_data("Language", lcfg(pref + "name"), "EXENAME", exe_file));
		return COMPILE_ERROR;
	}
	if (run_file) run_file->assign(exe_file);
	run_cmd = lang_str("run", lang, src_file);
	return COMPILE_OK;
}

bool tests_sort_func(cstr f1, cstr f2)
{
	int tn1 = CTest::parse_id(f1);
	int tn2 = CTest::parse_id(f2);
	return tn1 < tn2;
}

bool check_iofile_name(cstr fn)
{
	int n = (int)fn.size();
	int dot_cnt = 0;
	for (int i = 0; i < n; ++ i)
	{
		if (fn[i] >= '0' && fn[i] <= '9')
			continue;
		if (fn[i] >= 'a' && fn[i] <= 'z')
			continue;
		if (fn[i] >= 'A' && fn[i] <= 'Z')
			continue;
		if (fn[i] == '_')
			continue;
		if (fn[i] == '.')
		{
			++ dot_cnt;
			if (dot_cnt == 2)
				return false;
		}
		else
			return false;
	}
	return true;
}

