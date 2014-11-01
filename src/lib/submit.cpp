#include "common.hpp"

namespace bacs {

bool test_submit(cstr sid)
{
    CSubmit s(sid);
    return s.compile_and_test();
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
    DBRow rr = db_qres(format("select problem_id, lang, solution, need_info, acm from submit where submit_id = %s", sid.c_str()));
    if (rr.empty()) {
        not_found = true;
    }
    else
    {
        pid = rr[0]; lang = rr[1]; solution = rr[2]; need_info = (rr[3] == "1"); acm=rr[4]!="0";
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
    if (compile())
    {
        test();
        run_file.erase();
    }
    cleanup();
    store_result();
    ping(submit_result, sid, -1, pid, status_to_str(status));

    log.add(format("Finished testing [%s]", sid.c_str()),
        log.gen_data("Result", status_to_str(status),
        "Failed on test", i2s(test_num_failed),
        "Time used", format("%.3lf", max_time), "Memory used", format("%.3lf", max_memory)));

    return !fatal_error;
}

bool CSubmit::compile()
{
    if (!security_check()) {
        status = ST_SECURITY_VIOLATION;
        return false;
    }
    if (lcfg(str_lowercase(lang) + ".name") == "")
    {
        log.add("Error: unknown language!", log.gen_data("Language ID", lang));
        status = ST_INVALID_LANG;
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
        switch (res)
        {
        case COMPILE_ERROR:
            status = ST_COMPILE_ERROR;
            need_info = true;
            break;
        case COMPILE_TIME_LIMIT:
            status = ST_COMPILE_TIME_LIMIT;
            need_info = true;
            break;
        default:
            status = ST_SERVER_ERROR;
        }
        return false;
    }
    return true;
}

bool CSubmit::test()
{
    CProblem problem;
    if (!problem.init(pid, sid)) {
        log.add("Error initializing problem data!", log.gen_data("Problem ID", pid));
        status = ST_INVALID_PROBLEM;
        return false;
    }
    int st = ST_SERVER_ERROR;
    int no_ml = lcfgi(str_lowercase(lang) + ".no_memory_limit");
    if (no_ml)
        problem.set_no_memory_limit();
    bool res = problem.run_tests(run_cmd, lang, st, max_time, max_memory, test_num_failed, need_info ? (&info) : NULL, acm, test_results);
    status = st;
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
    constexpr std::size_t MAX_INFO_SIZE = 16384;

    if (info.size() > MAX_INFO_SIZE)
    {
        info = info.substr(0, MAX_INFO_SIZE);
        info += "\n... too many characters";
    }
    string s_info = need_info ? format(", info = '%s'", db.escape(info).c_str()) : "";
    if (acm)
        return db_query(format("update submit set result = %li, test_num_failed = %s, max_time_used = %lf, "
            "max_memory_used = %lf%s where submit_id = %s",
            status, (test_num_failed < 0) ? "NULL" : i2s(test_num_failed).c_str(), max_time, max_memory, s_info.c_str(), sid.c_str()));
    else
        return db_query(format("update submit set result = %li, school_result = '%s'"
            " , test_num_failed=%s, max_time_used = %lf,  max_memory_used = %lf%s where submit_id = %s",
            status, test_results.c_str(), (test_num_failed < 0)?"NULL" : i2s(test_num_failed).c_str(), max_time,
            max_memory, s_info.c_str(), sid.c_str()));

    return true;
}

bool CSubmit::security_check()
{
    return true;
}

} // bacs
