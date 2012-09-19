#include "config.hpp"

#include <boost/property_tree/ini_parser.hpp>

namespace bacs {

CCfgEngine config, langs_config;

string cfg(cstr key)
{
    return config.get(key);
}

string lcfg(cstr key)
{
    return langs_config.get(key);
}

int cfgi(cstr key)
{
    return s2i(config.get(key));
}

int lcfgi(cstr key)
{
    return s2i(langs_config.get(key));
}

double cfgd(cstr key)
{
    return s2d(config.get(key));
}

bool CCfgEngine::init(cstr _filename)
{
    filename = _filename;
    try
    {
        boost::property_tree::read_ini(filename, table);
        return true;
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        return false;
    }
}

string CCfgEngine::get(cstr key) const
{
    return table.get<std::string>(key, string());
}

} // bacs
