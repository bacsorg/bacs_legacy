#ifndef _BACS2_DB_H_
#define _BACS2_DB_H_

#include <unistd.h>
#include <mysql.h>
#include <string>
#include <vector>
#include "bacs2_def.h"
#include "bacs2_config.h"
#include "bacs2_log.h"

using namespace std;

typedef vector<string> DBRow;

DBRow db_qres(cstr s);
string db_qres0(cstr s);
bool db_query(cstr s);

class CDBQuery
{
private:
	CDBQuery() {}
	int _error;
	MYSQL_RES * result;
	int col_num;
public:
	CDBQuery(cstr s);
	~CDBQuery();
	int error() {return _error;}
	bool ok() {return _error == 0;}
	bool next_row(DBRow &row);
};

class CDBEngine
{
private:
	MYSQL* connection;
public:
	CDBEngine();
	~CDBEngine();
	bool connect();
	void close();
	string error();
	int get_errno();
	int query(cstr q);
	void log_error(cstr file, int line);
	void log_error(cstr file, int line, const LogEntryData &data);
	string escape(cstr s);
	MYSQL_RES* query_result();
};

extern CDBEngine db;

#endif
