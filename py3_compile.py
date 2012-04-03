#!/usr/bin/python3

import sys
import os
import shutil
import py_compile

if __name__=='__main__':
	src = sys.argv[1]
	dst = sys.argv[2]
	shutil.copyfile(src, dst)
	try:
		py_compile.compile(dst, doraise=True)
	except py_compile.PyCompileError as e:
		# thank to BACS authors
		# sys.exit(1) is not used
		os.unlink(dst)
		print(e.msg, file=sys.stderr)
		sys.exit(1)

