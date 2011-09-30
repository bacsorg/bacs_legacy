#ifndef _BACS2_TEMPFILES_H_
#define _BACS2_TEMPFILES_H_

#include <string>
#include <iostream>
#include <fstream>
#include "bacs2_def.h"
#include "bacs2_util.h"
#include "bacs2_config.h"

using namespace std;

class CTempFile
{
private:
	string _name;
	bool file_created;
public:
	bool create(cstr source = "");
	bool erase();
	inline void assign(cstr filename) {_name = filename; file_created = true;}
	string read(int max_size);
	inline string name() {return _name;}
	inline CTempFile() {file_created = false;}
	inline ~CTempFile() {erase();}
};

string gen_unique_filename();

#endif
