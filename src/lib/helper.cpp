#include <bacs/legacy/common.hpp>

namespace bacs {

string status_to_str(int status) {
  switch (status) {
    case ST_SERVER_ERROR:
      return "Internal server error";
    case ST_ACCEPTED:
      return "Accepted";
    case ST_COMPILE_ERROR:
      return "Compilation error";
    case ST_MEMORY_LIMIT:
      return "Memory limit exceeded";
    case ST_TIME_LIMIT:
      return "Time limit exceeded";
    case ST_RUNTIME_ERROR:
      return "Runtime error";
    case ST_WRONG_ANSWER:
      return "Wrong answer";
    case ST_PRESENTATION_ERROR:
      return "Presentation error";
    case ST_SECURITY_VIOLATION:
      return "Security violation";
    case ST_INVALID_PROBLEM:
      return "Invalid problem id";
    case ST_REALTIME_LIMIT:
      return "Real time limit exceeded";
    case ST_PENDING:
      return "Pending check";
    case ST_RUNNING:
      return "Running";
    case ST_OUTPUT_LIMIT:
      return "Output limit exceeded";
    case ST_COMPILE_TIME_LIMIT:
      return "Compilation time limit exceeded";
    case ST_INVALID_LANG:
      return "Invalid language";
    case ST_CHECKER_ERROR:
      return "Checker error";
    default:
      return "Unknown";
  }
}

bool lock_table() {
  string lock = db_qres0("select get_lock('olimp.submit_lock', 10)");
  return lock == "1";
}

void unlock_table() { db_qres("select release_lock('olimp.submit_lock')"); }

string lang_str(cstr key, cstr lang, cstr src) {
  string _lang = str_lowercase(lang);
  string s = lcfg(_lang + "." + key);
  string dir = lcfg(_lang + ".dir");
  string srcnoext = src.substr(0, src.find_last_of('.'));
  str_replace(s, "{src}", src);
  str_replace(s, "{dir}", dir);
  str_replace(s, "{src_noext}", srcnoext);
  return s;
}

}  // bacs
