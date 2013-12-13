#include <bunsan/config.hpp>

#include "common.hpp"

#include <bunsan/filesystem/fstream.hpp>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>

namespace bacs {

CProblem::CProblem()
{
    fatal_error = false;
}

bool CProblem::init(cstr _id, cstr submit_id)
{
    id = _id;
    string dir = cfg("general.problem_archive_dir") + "/" + id + "/";
    boost::filesystem::create_directories(dir);
    try
    {
        repository->extract(cfg("general.repository_prefix") + id + cfg("general.repository_suffix"), dir);
    }
    catch (std::exception &e)
    {
        log.add_error(__FILE__, __LINE__, format("Unable to extract package: %s", e.what()).c_str(), log.gen_data("Problem ID", id));
        return false;
    }
    string cf_fn = dir + "conf.txt";
    if (!cf.init(cf_fn)) return false;
    time_limit = s2d(cf.get("TL"), cfgd("general.default_time_limit"));
    memory_limit = s2d(cf.get("ML"), cfgd("general.default_memory_limit"));
    if (!init_iofiles()) return false;
    if (!init_checker()) return false;
    if (!init_tests(submit_id)) return false;
    return true;
}

CProblem::~CProblem()
{
    string dir = cfg("general.problem_archive_dir") + "/" + id + "/";
    system(format("rm -rf \"%s\"", dir.c_str()).c_str());
}

bool CProblem::run_tests(cstr run_cmd, cstr src_lang, int &result, double &max_time, double &max_memory, int &test_num_failed, string *info, bool acm, string& test_results)
{
    int i;
    int first_res = ST_ACCEPTED;
    max_time = 0;
    max_memory = 0;
    test_num_failed = -1;
    test_results.clear();
    boost::optional<boost::filesystem::path> verbose_path;
    BOOST_SCOPE_EXIT_ALL(&verbose_path)
    {
        try
        {
            if (verbose_path && boost::filesystem::exists(*verbose_path))
                boost::filesystem::remove_all(*verbose_path);
        }
        catch (...) {}
    };
    for (i = 0; i < (int)test.size(); ++i)
    {
        ping(running, "", i, id);
        double time_used, memory_used;
        const CTest &tt = test[i];
        if (tt.verbose)
        {
            if (!verbose_path)
                verbose_path = tt.verbose->parent_path();
            else
                BOOST_ASSERT(*verbose_path == tt.verbose->parent_path());
            boost::filesystem::create_directories(*tt.verbose);
            boost::filesystem::copy_file(tt.file_in, *tt.verbose / "input");
            if (tt.file_out)
                boost::filesystem::copy_file(*tt.file_out, *tt.verbose / "hint");
        }
        if (acm)
        {
            if (!run_test(tt, run_cmd, src_lang, result, time_used, memory_used, info))
                return false;
            if (time_used > max_time) max_time = time_used;
                if (memory_used > max_memory) max_memory = memory_used;
                    if (result != ST_ACCEPTED)
                    {
                        test_num_failed = test[i].id;
                        first_res = result;
                        break;
                    }
        }
        else
        {
            if (tests_for_check.empty() ||
                (i < (int)tests_for_check.size() && tests_for_check[i] == '1'))
            {
                if (!run_test(tt, run_cmd, src_lang, result, time_used, memory_used, info))
                    return false;
                if (time_used > max_time)
                    max_time = time_used;
                if (memory_used > max_memory)
                    max_memory = memory_used;
                if (test_results.size())
                    test_results.push_back(',');
                if (result != ST_ACCEPTED && first_res == ST_ACCEPTED)
                    first_res = result;
                test_results+=i2s(result);
            }
        }
        if (tt.verbose)
        {
            bunsan::filesystem::ofstream fout(*tt.verbose / "result");
            BUNSAN_FILESYSTEM_FSTREAM_WRAP_BEGIN(fout)
            {
                fout << "time used = " << time_used << '\n';
                fout << "memory used = " << memory_used << '\n';
                fout << "result = " << static_cast<ST>(result) << '\n';
            }
            BUNSAN_FILESYSTEM_FSTREAM_WRAP_END(fout)
            fout.close();
        }
    }
    result = first_res;
    if (verbose_path)
    {
        const boost::filesystem::path verbose_path_ar = verbose_path->parent_path() / (verbose_path->filename().string() + ".tgz");
        BOOST_SCOPE_EXIT_ALL(&verbose_path_ar)
        {
            try
            {
                boost::filesystem::remove(verbose_path_ar);
            }
            catch (...) {}
        };
        {
            if (boost::filesystem::exists(verbose_path_ar))
                boost::filesystem::remove_all(verbose_path_ar);
            std::ostringstream cmd;
            cmd << "cd " << verbose_path_ar.parent_path() << " && tar cf " << verbose_path_ar.filename() << ' ' << verbose_path->filename();
            const int ret = system(cmd.str().c_str());
            if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
                return false;
        }
        if (!cf_verbose_tests_server.empty())
        {
            std::ostringstream cmd;
            cmd << cf_verbose_tests_copy << ' ' << verbose_path_ar << ' ' << cf_verbose_tests_server << "/" << verbose_path_ar.filename().string();
            const int ret = system(cmd.str().c_str());
            if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
                return false;
        }
    }
    return true;
}

bool CProblem::init_tests(cstr submit_id)
{
    const boost::filesystem::path verbose_tmpdir = boost::filesystem::path(cfg("general.temp_dir")) / submit_id;
    if (boost::filesystem::exists(verbose_tmpdir))
        boost::filesystem::remove_all(verbose_tmpdir);
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
        if (t.id != INVALID_ID)
        {
            const bool verbose_test = db_qres0(
                    format(
                        "select 1 from verbose_test where submit_id = %s AND (test_id = %d OR test_id is NULL)",
                        submit_id.c_str(),
                        t.id
                    )
                ) == "1";
            if (verbose_test)
                t.verbose = verbose_tmpdir / boost::lexical_cast<std::string>(t.id);
            test.push_back(t);
        }
    }
    is_tests_init = true;
    return true;
}

bool CProblem::init_iofiles()
{
    input_fn = cf.get("input");
    if (input_fn == "")
        input_fn = "STDIN";
    if (!check_iofile_name(input_fn))
    {
        log.add("Error: input file is incorrect!", log.gen_data("Input file", input_fn));
        return false;
    }

    output_fn = cf.get("output");
    if (output_fn == "")
        output_fn = "STDOUT";
    if (!check_iofile_name(output_fn))
    {
        log.add("Error: output file is incorrect!", log.gen_data("Output file", output_fn));
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
    int res = run_fio(run_cmd, &ex, tt.file_in, s_file_out, limit_tl, limit_ml, t_used, m_used, input_fn, output_fn);
    time_used = (double)t_used / 1000;
    memory_used = (double)m_used / (1024 * 1024);
    bool ok = true;
    CTempFile s_out;
    s_out.assign(s_file_out);
    if (tt.verbose)
        boost::filesystem::copy_file(s_file_out, *tt.verbose / "output");
    if (res == RUN_OK && ex == 0)
    {
        if (info) *info = s_out.read(cfgi("general.max_info_size"));
        const std::string tt_file_out = tt.file_out ? *tt.file_out : "/dev/null";
        string checker_cmd = format("%s %s %s %s", checker.c_str(), tt.file_in.c_str(), s_file_out.c_str(), tt_file_out.c_str());
//      log.add_error(__FILE__, __LINE__, "Start checker!", log.gen_data("Run command", checker_cmd));
        string checker_out;
        int res2 = run(checker_cmd, &ex, true, "", checker_out, cf_checker_timeout);
        if (res2 == RUN_FAILED || res2 == RUN_ABNORMAL_EXIT) {
            log.add_error(__FILE__, __LINE__, "Error: cannot run checker!", log.gen_data("Run command", checker_cmd));
            result = ST_CHECKER_ERROR;
            ok = false;
        }
        else if (res2 == RUN_TIMEOUT) {
            log.add_error(__FILE__, __LINE__, "Error: checker timed out!", log.gen_data("Run command", checker_cmd, "Checker output", checker_out));
            result = ST_CHECKER_ERROR;
            ok = false;
        }
        else
        {
            if (tt.verbose)
            {
                {
                    bunsan::filesystem::ofstream fout(*tt.verbose / "checker_output");
                    BUNSAN_FILESYSTEM_FSTREAM_WRAP_BEGIN(fout)
                    {
                        fout << checker_out;
                    }
                    BUNSAN_FILESYSTEM_FSTREAM_WRAP_END(fout)
                    fout.close();
                }
            }
            if (info && checker_out != "") *info += "\n === CHECKER OUTPUT ===\n" + checker_out;
            if (ex == CHECK_RES_OK) result = ST_ACCEPTED;
            else if (ex == CHECK_RES_WA || ex == CHECK_RES_WA_OLD) result = ST_WRONG_ANSWER;
            else if (ex == CHECK_RES_PE) result = ST_PRESENTATION_ERROR;
            else {
                log.add_error(__FILE__, __LINE__, "Error: checker has returned unexpected result!", log.gen_data("Result code", i2s(ex)));
                result = ST_CHECKER_ERROR;
                ok = false;
            }
        }
    }
    else
    {
        if (res == RUN_FAILED)
        {
            log.add_error(__FILE__, __LINE__, "Error: cannot run solution!", log.gen_data("Run command", run_cmd));
            result = ST_SERVER_ERROR;
            ok = false;
        }
        else if (res == RUN_TIMEOUT) result = ST_TIME_LIMIT;
        else if (res == RUN_OUT_OF_MEMORY) result = ST_MEMORY_LIMIT;
        else if (res == RUN_REALTIMEOUT) result = ST_REALTIME_LIMIT;
        else if (res == RUN_OUTPUT_LIMIT) result = ST_OUTPUT_LIMIT;
        else if (res == RUN_ABNORMAL_EXIT || (res == RUN_OK && ex != 0)) result = ST_RUNTIME_ERROR;
        else
        {
            log.add_error(__FILE__, __LINE__, "Error: unexpected run() result!", log.gen_data("Result", i2s(res)));
            result = ST_SERVER_ERROR;
            ok = false;
        }
    }
    return ok;
}

void CProblem::set_no_memory_limit()
{
    memory_limit = 0.;
}

} // bacs
