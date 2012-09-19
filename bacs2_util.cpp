#include "bacs2_util.h"
#include "bacs2_log.h"

#include <cstring>

static const int MAX_BIG_BUF = 1<<16 + 1;
static char big_buf[MAX_BIG_BUF];

string get_time_str()
{
    struct tm *newTime;
    time_t szClock;

    time(&szClock);
    newTime = localtime(&szClock);
    string res = asctime(newTime);
    res = res.substr(0, res.length() - 1);
    return res;
}

string i2s(int x)
{
    char buf[64];
    sprintf(buf, "%d", x);
    return buf;
}

int s2i(cstr s, int default_value)
{
    int x = default_value;
    sscanf(s.c_str(), "%d", &x);
    return x;
}

double s2d(cstr s, double default_value)
{
    double x = default_value;
    sscanf(s.c_str(), "%lf", &x);
    return x;
}

string format(const char *q, ...)
{
    va_list args;
    va_start(args, q);
    return vformat(q, args);
}

string vformat(const char *q, va_list args)
{
    vsnprintf(big_buf, MAX_BIG_BUF, q, args);//TODO will not write more than MAX_BIG_BUF
    big_buf[MAX_BIG_BUF-1] = '\0';
    string res = big_buf;
    return res;
}

void parse_str(cstr s, char x, string &s1, string &s2)
{
    int p = (int)s.find(x);
    if (p < 0)
    {
        s1 = s;
        s2 = "";
    }
    else
    {
        s1 = s.substr(0, p);
        s2 = s.substr(p + 1);
    }
}

bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void trim(string &s)
{
    int x, y;
    x = 0;
    while (x < (int)s.length() && is_whitespace(s[x])) ++x;
    y = (int)s.length() - 1;
    while (y >= 0 && is_whitespace(s[y])) --y;
    if (x <= y)
        s = s.substr(x, y - x + 1);
    else s = "";
}

string str_lowercase(string s)
{
    int i;
    for (i = 0; i < (int)s.length(); ++i)
        s[i] = tolower(s[i]);
    return s;
}

bool file_exists(cstr fn)
{
  struct stat stFileInfo;
  bool blnReturn;
  int intStat;

  // Attempt to get the file attributes
  intStat = stat(fn.c_str(), &stFileInfo);
  if(intStat == 0) {
    // We were able to get the file attributes
    // so the file obviously exists.
    blnReturn = !S_ISDIR(stFileInfo.st_mode);
  } else {
    // We were not able to get the file attributes.
    // This may mean that we don't have permission to
    // access the folder which contains this file. If you
    // need to do that level of checking, lookup the
    // return values of stat which will give you
    // more details on why stat failed.
    blnReturn = false;
  }

  return (blnReturn);
/*
    WIN32_FIND_DATA data;
    HANDLE h = FindFirstFile(fn.c_str(), &data);
    if (h == INVALID_HANDLE_VALUE) return false;
    FindClose(h);
    return true;
*/

}

void str_replace(string &s, cstr subs, cstr news)
{
    int x = 0;
    while (1)
    {
        int k = (int)s.find(subs, x);
        if (k < 0) break;
        s = s.replace(k, subs.length(), news);
        x = k + (int)subs.length();
    }
}



#include <cstdlib>

int random(int max_num)
{
    long int rnd = random();
    return (int)((double)rnd / (double)RAND_MAX * max_num);
}

string dir_from_filename(cstr fn)
{
    int k = (int)fn.find_last_of('/');
    if (k >= 0) return fn.substr(0, k);
    return fn;
}

string name_from_filename(cstr fn)
{
    int k = (int)fn.find_last_of('/');
    if (k >= 0) return fn.substr(k + 1);
    return fn;
}

bool find_files(FileList &list, cstr fn, cstr ext)
{
    list.clear();
    string dir = dir_from_filename(fn) + "/";
    DIR *dp;
    struct dirent *dirp;
    if ((dp  = opendir(dir.c_str())) == NULL)
    {
        //so i think file not found
        return true;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        string nfn = dir + dirp->d_name;
        if ((int)nfn.find(ext) < 0)
            continue;
        if (file_exists(nfn))
            list.push_back(nfn);
    }
    closedir(dp);
    return true;
/*
    HANDLE h;
    WIN32_FIND_DATA data;

    list.clear();
    string dir = dir_from_filename(fn) + "\\";
    h = FindFirstFile(fn.c_str(), &data);
    if (h != INVALID_HANDLE_VALUE)
    {
        find_files_add(list, dir, data);
        while (FindNextFile(h, &data) != 0)
        {
            find_files_add(list, dir, data);
        }
        FindClose(h);
        if (GetLastError() != ERROR_NO_MORE_FILES) return false;
    }
    else
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) return false;
    }
    return true;
*/
}

bool delete_file(cstr filename)
{
    return remove(filename.c_str()) != 0;
}

void chdir(cstr dir)
{
    chdir(dir.c_str());
}

bool success_ret(const char *file, int line, int ret)
{
    if (ret)
    {
        log.add_error(file, line, strerror(ret));
        return false;
    }
    else
        return true;
}

