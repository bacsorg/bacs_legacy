#include "bacs2_config.h"

CCfgEngine config;

string cfg(cstr key)
{
	return config.get(key);
}

int cfgi(cstr key)
{
	return s2i(config.get(key));
}

double cfgd(cstr key)
{
	return s2d(config.get(key));
}

void log_error(cstr err, cstr fn, int line)
{
	fprintf(stderr, "%s (%s, line %d)\n", err.c_str(), fn.c_str(), line);
}

void CCfgEngine::parse_line(cstr s)
{
	if (s == "" || s[0] == ';') return;
	if (s[0] == '[') {
		if (s[s.length() - 1] != ']') {
			log_error("Error: invalid header in configuration file", filename, line);
			return;
		}
		pref1 = s.substr(1, s.length() - 2);
		pref2 = "";
	}
	else if (s[0] == '(') {
		if (s[s.length() - 1] != ')') {
			log_error("Error: invalid header in configuration file", filename, line);
			return;
		}
		pref2 = s.substr(1, s.length() - 2);
	}
	else
	{
		int x = (int)s.find('=');
		bool ok = true;
		if (!x) ok = false;
		else
		{
			string k, v;
			parse_str(s, '=', k, v);
			if (k.empty()) ok = false;
			else
			{
				string key = k;
				if (pref2 != "") key = pref2 + "." + key;
				if (pref1 != "") key = pref1 + "." + key;
				key = str_lowercase(key);
				table.insert(make_pair(key, v));
			}
		}
		if (!ok)
			log_error("Error: invalid option in configuration file", filename, line);
	}
}

bool CCfgEngine::init(cstr _filename)
{
	FILE *f;

	filename = _filename;
	f = fopen( filename.c_str(), "r");
	if (!f) return false;
	line = 0;
	pref1 = "";
	pref2 = "";
	while (!feof(f)) {
		char buf[1024];

		++line;
		fgets(buf, 1023, f);
		string s = buf;
		trim(s);
		parse_line(s);
	}
	fclose(f);
	return true;
}
