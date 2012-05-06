#ifndef _BACS2_CONFIG_H_
#define _BACS2_CONFIG_H_

#include <unistd.h>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include "bacs2_def.h"
#include "bacs2_util.h"

using namespace std;

class CCfgEngine
{
private:
	boost::property_tree::ptree table;
	int line;
	string filename;
public:
	bool init(cstr _filename);
	string get(cstr key) const;
};

string cfg(cstr key);
string lcfg(cstr key);
int cfgi(cstr key);
int lcfgi(cstr key);
double cfgd(cstr key);

extern CCfgEngine config;
extern CCfgEngine langs_config;

#endif
