#include "bacs2_log.h"

CLogEngine log;
LogEntryData _no_data;

void CLogEngine::add_error(cstr file, int line, cstr text, LogEntryData data)
{
    data.push_back(make_pair("Source", file + ", line " + i2s(line)));
    add(text, data);
}

void CLogEngine::add(cstr text, const LogEntryData &data)
{
    if (!is_init) init();
    if (!is_cout && !is_fout) return;
    string s = "";
    string time = get_time_str();
    s += format("\n[%s] %s\n", time.c_str(), text.c_str());
    LogEntryData::const_iterator it;
    for (it = data.begin(); it != data.end(); ++it)
    {
        s += format("    %s: %s\n", it->first.c_str(), it->second.c_str());
    }
    if (is_cout) printf("%s", s.c_str());
    if (is_cout) {
        fprintf(f, "%s", s.c_str());
        fflush(f);
    }
}

LogEntryData CLogEngine::gen_data(cstr k0, cstr v0, cstr k1, cstr v1, cstr k2, cstr v2, cstr k3, cstr v3)
{
    LogEntryData res;
    if (k0 != "") res.push_back(make_pair(k0, v0));
    if (k1 != "") res.push_back(make_pair(k1, v1));
    if (k2 != "") res.push_back(make_pair(k2, v2));
    if (k3 != "") res.push_back(make_pair(k3, v3));
    return res;
}

CLogEngine::CLogEngine()
{
    is_init = false;
}

CLogEngine::~CLogEngine()
{
    close();
}

bool CLogEngine::init()
{
    if (is_init) close();
    string s = cfg("log.output");
    if (s == "none") {
        is_cout = false;
        is_fout = false;
    }
    else if (s == "console") {
        is_cout = true;
        is_fout = false;
    }
    else if (s == "file") {
        is_cout = false;
        is_fout = true;
    }
    else if (s == "both") {
        is_cout = true;
        is_fout = true;
    }
    else return false;
    if (is_fout) {
        string fn = cfg("log.log_file");
        f = fopen(fn.c_str(), "a");
        if (!f) return false;
    }
    is_init = true;
    return true;
}

void CLogEngine::close()
{
    if (is_init)
    {
        if (is_fout) fclose(f);
        is_init = false;
    }
}

void CLogEngine::add_working_notify()
{
    if (is_init && is_cout)
    {
        printf(".");
        fflush(stdout);
    }
}
