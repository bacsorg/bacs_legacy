#ifndef _BACS2_PIPES_H_
#define _BACS2_PIPES_H_

#include <unistd.h>
#include <cstdio>
#include "bacs2_def.h"
#include "bacs2_log.h"

#define HANDLE int

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
	int h_in() { return fd[1]; }
	int h_out() { return fd[0]; }
};

#endif
