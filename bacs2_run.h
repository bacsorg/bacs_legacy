#ifndef _BACS2_RUN_H_
#define _BACS2_RUN_H_

#define _CRT_SECURE_NO_DEPRECATE
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "bacs2_def.h"
#include "bacs2_util.h"
#include "bacs2_log.h"
#include "bacs2_tempfiles.h"

extern int cf_job_query_period;
extern int cf_max_idle_time;

int run(cstr cmd, int *exit_code, bool use_pipes, cstr data_in, string &data_out, int timeout, int memory_limit, int &time_used, int &memory_used, const bool redirect_stderr = true);
int run(cstr cmd, int *exit_code, bool use_pipes, cstr data_in, string &data_out, int timeout = 0, int memory_limit = 0, const bool redirect_stderr = true );
int run_fio(cstr cmd, int *exit_code, cstr data_in, string &data_out, int timeout, int memory_limit, int &time_used, int &memory_used, cstr input_fn, cstr output_fn );

#endif
