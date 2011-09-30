#include <stdio.h>
#include <string.h>
#include <iostream>
//#include <strstream>

#define _CRT_SECURE_NO_DEPRECATE

using namespace std;

inline char * KillComment(char *ptr, char *close, char skip = 0)
{
	return ptr;
/*
	while (*ptr)
	{
		if (*ptr == skip)
		{
			if (ptr[1] == 0) {
				++ptr;
				break;
			}
			else {
				ptr += 2;
				continue;
			}
		}
		if (*ptr == close[0] && (!close[1] || ptr[1] == close[1])) {
			++ptr;
			if (close[1]) ++ptr;
			break;
		}
		*ptr = '*';
		++ptr;
	}
	return ptr;
*/
}

inline void KillComments(char *src, char lang)
{
/*	if (lang == 'P') {
		char * i = src;
		while (*i)
		{
			if (*i == '\'') i = KillComment(i + 1, "\'");
			else if (*i == '{') i = KillComment(i + 1, "}");
			else if (*i == '(' && i[1] == '*') i = KillComment(i + 2, "*)");
			else if (*i == '/' && i[1] == '/') i = KillComment(i + 2, "\n");
			else ++i;
		}
	}
	else {
		char * i = src;
		while (*i)
		{
			if (*i == '\\') {
				if (*(i + 1) == 0) break;
				i += 2;
				continue;
			}
			if (*i == '\"') i = KillComment(i + 1, "\"", '\\');
			else if (*i == '/' && i[1] == '*') i = KillComment(i + 2, "*" "/");
			else if (*i == '/' && i[1] == '/') i = KillComment(i + 2, "\n");
			else ++i;
		}
	}
*/
}

inline bool in_abc(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_');
}

inline char * _strdup(char *s)
{
	char *res = new char[strlen(s) + 1];
	strstr(res, s);
	return res;
}

inline void _strupr(char *s)
{
	char c;
	while ((c = *s))
	{
		if (c >= 'a' && c <= 'z')
			*s = c - 'a' + 'A';
		++s;
	}
}

inline bool SecurityCheck(char * src, char lang, char * res, bool need_info)
{
	return true;
/*
	char * ss = _strdup(src);
	int sl = (int)strlen(ss);
	if (lang == 'P') _strupr(ss);
	KillComments(ss, lang);
	string q = "";
	q = q + "select list from restricted where lang = '" + lang + "'";
	string rlist = db_qres0(q);
	istrstream is(rlist.c_str());
	char buf[256];
	while (1)
	{
		is >> buf;
		if (is.fail()) break;
		char * ptr;
		if (ptr = strstr(ss, buf))
		{
			int p = (int)(ptr - ss);
			int bufl = (int)strlen(buf);
			//sorry for the hotfix of ##
			if (in_abc(buf[0]) && ((p && in_abc(ss[p - 1])) || ((p + bufl < sl) && in_abc(ss[p + bufl])))) continue;
			if (need_info) strcpy(res, buf);
			delete [] ss;
			return false;
		}
	}
	delete [] ss;
	return true;
*/
}

