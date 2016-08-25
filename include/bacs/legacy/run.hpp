#pragma once

#include <bacs/legacy/def.hpp>

#include <bacs/legacy/log.hpp>
#include <bacs/legacy/tempfiles.hpp>
#include <bacs/legacy/util.hpp>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

namespace bacs {

extern int cf_job_query_period;
extern int cf_max_idle_time;

int run(cstr cmd, int *exit_code, bool use_pipes, cstr data_in,
        string &data_out, int timeout, int memory_limit, int &time_used,
        int &memory_used, const bool redirect_stderr = true);
int run(cstr cmd, int *exit_code, bool use_pipes, cstr data_in,
        string &data_out, int timeout = 0, int memory_limit = 0,
        const bool redirect_stderr = true);
int run_fio(cstr cmd, int *exit_code, cstr data_in, string &data_out,
            int timeout, int memory_limit, int &time_used, int &memory_used,
            cstr input_fn, cstr output_fn);

}  // bacs
