#include "bacs2_db.h"

//#pragma comment(lib, "mysqlclient")
//#pragma link "mysqlclient"

CDBEngine db;

DBRow db_qres(cstr s)
{
	CDBQuery q(s);
	DBRow r;
	q.next_row(r);
	return r;
}

string db_qres0(cstr s)
{
	DBRow r = db_qres(s);
	if (r.empty()) return "";
	return r[0];
}

bool db_query(cstr s)
{
	int res = db.query(s);
	if (res) {
		db.log_error(__FILE__, __LINE__, log.gen_data("Query", s));
		return false;
	}
	return true;
}

CDBEngine::CDBEngine()
{
	connection = NULL;
}

CDBEngine::~CDBEngine()
{
}

bool CDBEngine::connect()
{
	close();
	connection = mysql_init(NULL);
	mysql_options(connection, MYSQL_OPT_RECONNECT, cfg("db.reconnect").c_str());
	if (!mysql_real_connect(connection, cfg("db.host").c_str(), cfg("db.user").c_str(), cfg("db.pass").c_str(), cfg("db.database").c_str(), cfgi("db.port"), NULL, 0))
	{
		log_error(__FILE__, __LINE__);
		close();
		return false;
	}
	return true;
}

void CDBEngine::close()
{
	if (connection) mysql_close(connection);
	connection = NULL;
}

string CDBEngine::error()
{
	if (!connection) return "Not connected to the database.";
	string res = mysql_error(connection);
	return res;
}

int CDBEngine::get_errno()
{
	if (!connection) return 0;
	return mysql_errno(connection);
}

int CDBEngine::query(cstr q)
{
	if (!connection) return 1;
	return mysql_query(connection, q.c_str());
}

MYSQL_RES* CDBEngine::query_result()
{
	if (!connection) return NULL;
	return mysql_store_result(connection);
}

void CDBEngine::log_error(cstr file, int line, const LogEntryData &data)
{
	log.add_error(file, line, "MySQL error: " + error(), data);
}

void CDBEngine::log_error(cstr file, int line)
{
	log_error(file, line, log.no_data());
}

string CDBEngine::escape(cstr s)
{
	if (!connection) return "";
	char *buf = new char[s.length() * 2 + 1];
	mysql_real_escape_string(connection, buf, s.c_str(), (unsigned long)s.length());
	string res = buf;
	delete [] buf;
	return res;
}

CDBQuery::CDBQuery(cstr s)
{
	result = NULL;
	int flag = db.query(s);
	if (flag)
	{
		_error = db.get_errno();
		db.log_error(__FILE__, __LINE__, log.gen_data("Query", s));
		return;
	}
	result = db.query_result();
	col_num = mysql_num_fields(result);
	_error = 0;
}

bool CDBQuery::next_row(DBRow &row)
{
	if (!result) return false;
	MYSQL_ROW rr = mysql_fetch_row(result);
	unsigned long *ll = mysql_fetch_lengths(result);
	if (!rr) return false;
	row.clear();
	row.reserve(col_num);
	int i;
	for (i = 0; i < col_num; ++i)
	{
		if (!rr[i]) row.push_back("NULL");
		else {
			string s;
			s.assign(rr[i], ll[i]);
			row.push_back(s);
		}
	}
	return true;
}

CDBQuery::~CDBQuery()
{
	if (result) mysql_free_result(result);
}
