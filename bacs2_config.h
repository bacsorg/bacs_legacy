#ifndef _BACS2_CONFIG_H_
#define _BACS2_CONFIG_H_

#include <unistd.h>
#include <string>
#include <map>
#include "bacs2_def.h"
#include "bacs2_util.h"

using namespace std;

typedef map<string, string> CfgTable;

class CCfgEngine
{
private:
	CfgTable table;
	int line;
	string filename;
	string pref1, pref2;
	void parse_line(cstr s);
public:
	bool init(cstr _filename);
	inline string get(cstr key) {return table[key];}
};

string cfg(cstr key);
int cfgi(cstr key);
double cfgd(cstr key);

extern CCfgEngine config;

#endif
