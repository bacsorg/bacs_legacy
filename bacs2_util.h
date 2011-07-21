#ifndef _BACS2_UTIL_H_
#define _BACS2_UTIL_H_

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_RAND_S

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "bacs2_def.h"

using namespace std;

typedef vector<string> FileList;

string get_time_str();
string i2s(int x);
int s2i(cstr s, int default_value = 0);
double s2d(cstr s, double default_value = 0);
string format(const char *q, ...);
string vformat(const char *q, va_list args);
void parse_str(cstr s, char x, string &s1, string &s2);
void trim(string &s);
string str_lowercase(string s);
bool file_exists(cstr fn);
void str_replace(string &s, cstr subs, cstr news);
int random(int max_num);
bool find_files(FileList &f, cstr fn, cstr ext);
string dir_from_filename(cstr fn);
string name_from_filename(cstr fn);
bool delete_file(cstr filename);
void chdir(cstr dir);

#endif
