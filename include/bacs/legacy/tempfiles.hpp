#pragma once

#include <bacs/legacy/def.hpp>

#include <bacs/legacy/config.hpp>
#include <bacs/legacy/util.hpp>

#include <fstream>
#include <iostream>
#include <string>

namespace bacs {

using namespace std;

class CTempFile {
 private:
  string _name;
  bool file_created;

 public:
  bool create(cstr source = "");
  bool erase();
  inline void assign(cstr filename) {
    _name = filename;
    file_created = true;
  }
  string read(int max_size);
  inline void detach() { file_created = false; }
  inline string name() { return _name; }
  inline CTempFile() { file_created = false; }
  inline ~CTempFile() { erase(); }
};

string gen_unique_filename();

}  // bacs
