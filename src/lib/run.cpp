#include <bacs/legacy/run.hpp>

#include <cstdlib>

namespace bacs {

int _run(cstr cmd, int *exit_code, cstr fn_in, cstr fn_out, int timeout,
         int memory_limit, int &time_used, int &memory_used,
         const bool redirect_stderr = true) {
  string redir = "no ";
  if (redirect_stderr) redir = "yes ";
  string run_cmd = (cfg("general.limit_run_exe") + " " +
                    cfg("general.test_dir") + " " + fn_in + " " + fn_out + " " +
                    i2s(timeout) + " " + i2s(memory_limit) + " " + redir + cmd);
  // log.add(run_cmd.c_str());
  int ret = system(run_cmd.c_str());
  if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0) return RUN_FAILED;
  FILE *f = fopen(cfg("general.limit_run_result_file").c_str(), "r");
  if (!f) {
    log.add_error(__FILE__, __LINE__, "Cannot open run result!",
                  log.gen_data("Command string", cmd));
    return RUN_FAILED;
  }
  int res = -1;
  int ex_code = -1;
  if (fscanf(f, "%d %d %d %d", &res, &ex_code, &memory_used, &time_used) != 4) {
    log.add_error(__FILE__, __LINE__, "Cannot read run result!",
                  log.gen_data("Command string", cmd));
    res = RUN_FAILED;
  }
  if (exit_code) *exit_code = ex_code;
  fclose(f);
  return res;
}

int run_use_pipes(cstr cmd, int *exit_code, cstr data_in, string &data_out,
                  int timeout, int memory_limit, int &time_used,
                  int &memory_used, const bool redirect_stderr = true) {
  CTempFile tf_in, tf_out;
  if (!tf_in.create(data_in)) {
    log.add_error(__FILE__, __LINE__,
                  "Error: cannot write input data to temp file!");
    return RUN_FAILED;
  }
  if (!tf_out.create("")) {
    log.add_error(__FILE__, __LINE__, "Error: cannot create output temp file!");
    return RUN_FAILED;
  }
  int result = _run(cmd, exit_code, tf_in.name(), tf_out.name(), timeout,
                    memory_limit, time_used, memory_used, redirect_stderr);
  data_out = tf_out.read(cfgi("general.max_run_out_size"));
  return result;
  /*  if (result != RUN_FAILED)
      {
          log.add_error(__FILE__, __LINE__, "Error: cannot read output data from
     pipe!");
          return RUN_FAILED;
      }
  */
}

/*
HANDLE file_open_read(cstr filename)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;
    sa.lpSecurityDescriptor = NULL;
    return CreateFile(filename.c_str(), FILE_READ_DATA, 0, &sa, OPEN_EXISTING,
FILE_ATTRIBUTE_NORMAL, NULL);
}

HANDLE file_open_write(cstr filename)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;
    sa.lpSecurityDescriptor = NULL;
    return CreateFile(filename.c_str(), FILE_WRITE_DATA, 0, &sa, CREATE_ALWAYS,
FILE_ATTRIBUTE_NORMAL, NULL);
}
*/
int run_use_files(cstr cmd, int *exit_code, cstr file_in, string &file_out,
                  int timeout, int memory_limit, int &time_used,
                  int &memory_used, const bool redirect_stderr = true) {
  CTempFile tf_out;
  if (!tf_out.create("")) {
    log.add_error(__FILE__, __LINE__, "Error: cannot create output temp file!");
    return RUN_FAILED;
  }
  file_out = tf_out.name();
  int result = _run(cmd, exit_code, file_in, file_out, timeout, memory_limit,
                    time_used, memory_used, redirect_stderr);
  return result;
}

int run(cstr cmd, int *exit_code, bool use_pipes, cstr data_in,
        string &data_out, int timeout, int memory_limit, int &time_used,
        int &memory_used, const bool redirect_stderr) {
  if (use_pipes)
    return run_use_pipes(cmd, exit_code, data_in, data_out, timeout,
                         memory_limit, time_used, memory_used, redirect_stderr);
  else
    return run_use_files(cmd, exit_code, data_in, data_out, timeout,
                         memory_limit, time_used, memory_used, redirect_stderr);
}

int run(cstr cmd, int *exit_code, bool use_pipes, cstr data_in,
        string &data_out, int timeout, int memory_limit,
        const bool redirect_stderr) {
  int dummy1, dummy2;
  return run(cmd, exit_code, use_pipes, data_in, data_out, timeout,
             memory_limit, dummy1, dummy2, redirect_stderr);
}

int run_fio(cstr cmd, int *exit_code, cstr file_in, string &file_out,
            int timeout, int memory_limit, int &time_used, int &memory_used,
            cstr input_fn, cstr output_fn) {
  string in_fn;
  CTempFile tf_in, tf_out;
  string full_output_fn = cfg("general.test_dir") + "/" + output_fn;
  string full_input_fn = cfg("general.test_dir") + "/" + input_fn;

  if (input_fn != "STDIN") {
    if (!tf_in.create("")) {
      log.add_error(__FILE__, __LINE__,
                    "Error: cannot create input temp file!");
      return RUN_FAILED;
    }
    int ret = system(
        format("cp -f %s %s", file_in.c_str(), full_input_fn.c_str()).c_str());
    if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0) return RUN_FAILED;
    ret = chmod(full_input_fn.c_str(), 0664);
    if (!success_ret(__FILE__, __LINE__, ret)) return RUN_FAILED;
    in_fn = tf_in.name();
  } else {
    in_fn = file_in;
  }
  if (!tf_out.create("")) {
    log.add_error(__FILE__, __LINE__, "Error: cannot create output temp file!");
    return RUN_FAILED;
  }
  if (output_fn != "STDOUT") {
    // creating and clearing output file
    FILE *f = fopen(full_output_fn.c_str(), "wb");
    fclose(f);
    int ret = chmod(full_output_fn.c_str(), 0662);
    if (!success_ret(__FILE__, __LINE__, ret)) return RUN_FAILED;
  }
  // preparing done, lets run
  file_out = tf_out.name();
  tf_out.detach();
  int result = _run(cmd, exit_code, in_fn, file_out, timeout, memory_limit,
                    time_used, memory_used, false);
  if (output_fn != "STDOUT") {
    // need read from output
    if (file_exists(full_output_fn)) {
      int ret = rename(full_output_fn.c_str(), file_out.c_str());
      if (!success_ret(__FILE__, __LINE__, ret)) return RUN_FAILED;
    } else {
      FILE *f = fopen(file_out.c_str(), "wb");
      fclose(f);
    }
  }

  if (input_fn != "STDIN") {
    tf_in.erase();
    int ret = unlink(full_input_fn.c_str());
    if (!success_ret(__FILE__, __LINE__, ret)) return RUN_FAILED;
  }
  return result;
}

}  // basc
