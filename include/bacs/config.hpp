#pragma once

#include <unistd.h>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include "def.hpp"
#include "util.hpp"

namespace bacs {

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

} // bacs
