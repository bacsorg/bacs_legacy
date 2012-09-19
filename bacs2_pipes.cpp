#include "bacs2_pipes.h"

CPipe::CPipe()
{
    is_init = false;
    fd[0] = fd[1] = NULL;
}

CPipe::~CPipe()
{
    pclose();
}

bool CPipe::pinit()
{
    pclose();
    is_init = pipe(fd);
    return is_init;
}

void CPipe::pclose()
{
    if (is_init)
    {
        close(fd[0]);
        close(fd[1]);
        is_init = false;
    }
}

bool CPipe::pwrite(cstr s)
{
    if (!is_init) return false;
    int written = write(fd[1], s.c_str(), (int)s.length());
    if (written != (int)s.length())
    {
        log.add_error(__FILE__, __LINE__, "Error: pipe write error!");
        return false;
    }
    return true;
}

bool CPipe::pread(string &s)
{
    s = "";
    if (!is_init) return false;
/*  DWORD bytes_avail, bytes_read;
    if (!PeekNamedPipe(_h_in, NULL, 0, NULL, &bytes_avail, NULL)) return false;
    if (bytes_avail > 0)
    {
        char *buf = new char[bytes_avail + 1];
        if (!ReadFile(_h_in, buf, bytes_avail, &bytes_read, NULL)) {
            delete [] buf;
            return false;
        }
        if (bytes_read != bytes_avail) {
            delete [] buf;
            log.add_error(__FILE__, __LINE__, "Error: pipe read error!");
            return false;
        }
        s.assign(buf, bytes_read);
        delete [] buf;
    }
*/  return true;
}
