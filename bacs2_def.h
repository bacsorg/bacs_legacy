#ifndef _BACS2_DEF_H_
#define _BACS2_DEF_H_

#define _CRT_SECURE_NO_DEPRECATE

#include <string>
#include <sstream>

using namespace std;

typedef const string & cstr;

#define VERSION "2.2.alpha *fu_bsd"

#define CONFIG_FILE_NAME "bacs2.conf"

#define ST_SERVER_ERROR 0
#define ST_ACCEPTED 1
#define ST_COMPILE_ERROR 2
#define ST_MEMORY_LIMIT 3
#define ST_TIME_LIMIT 4
#define ST_RUNTIME_ERROR 5
#define ST_WRONG_ANSWER 6
#define ST_PRESENTATION_ERROR 7
#define ST_SECURITY_VIOLATION 8
#define ST_INVALID_PROBLEM 9
#define ST_PENDING 100
#define ST_RUNNING 101

#define STATE_NO 0
#define STATE_COMPILING 1

#define SOURCE_TYPE_SUBMIT 1
#define SOURCE_TYPE_CHECKER 2

#define RUN_OK 0
#define RUN_FAILED 1
#define RUN_TIMEOUT 2
#define RUN_OUT_OF_MEMORY 3
#define RUN_ABNORMAL_EXIT 4

#define CHECK_RES_OK 0
#define CHECK_RES_WA 5
#define CHECK_RES_PE 4
#define CHECK_RES_WA_OLD 2

#define COMPILE_OK 0
#define COMPILE_ERROR 1
#define COMPILE_FAILED 2

#define INVALID_ID 9999

#define JOB_QUERY_PERIOD 500

#endif
