#pragma once

#include <bacs/legacy/config.hpp>
#include <bacs/legacy/def.hpp>
#include <bacs/legacy/util.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace bacs {

using namespace std;

typedef vector<pair<string, string> > LogEntryData;

extern LogEntryData _no_data;

class CLogEngine {
 private:
  bool is_init, is_cout, is_fout;
  FILE *f;

 public:
  CLogEngine();
  ~CLogEngine();
  bool init();
  void close();
  inline const LogEntryData &no_data() { return _no_data; }
  void add(cstr text, const LogEntryData &data = _no_data);
  void add_error(cstr file, int line, cstr text, LogEntryData data = _no_data);
  void add_working_notify();
  LogEntryData gen_data(cstr k0 = "", cstr v0 = "", cstr k1 = "", cstr v1 = "",
                        cstr k2 = "", cstr v2 = "", cstr k3 = "", cstr v3 = "");
};

extern CLogEngine log;

}  // bacs
