#pragma once

#include <unistd.h>
#include <cstdio>
#include "def.hpp"
#include "log.hpp"

namespace bacs {

typedef int HANDLE;

class CPipe
{
private:
    bool is_init;
    int fd[2];
public:
    CPipe();
    ~CPipe();
    bool pinit();
    void pclose();
    bool pwrite(cstr s);
    bool pread(string &s);
    inline int h_in() { return fd[1]; }
    inline int h_out() { return fd[0]; }
};

} // bacs
