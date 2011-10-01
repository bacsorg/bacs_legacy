#!/usr/bin/env bash
set -e

sources="$(dirname "$0")"

if [[ ${sources:0:1} != / ]]
then
	sources="$PWD/$sources"
fi


config="${1?You have to specify config file}"
. "$config"
shift

for i
do
	dst="$i"
	if [[ ${dst:0:1} != / ]]
	then
		dst="$PWD/$dst"
	fi
	if [ -e "$dst" ]
	then
		echo "Remove \"$dst\" if you want to init environment in it" >&2
	else
		mkdir -p "$dst"
		cd "$dst"
		cmake "$sources"
		make
		mkdir Test Temp
		cat >test_c++ <<EOF
#!/bin/sh
exec "\$@"

EOF
		chmod +x test_c++
		cat >java.policy <<EOF
grant {
};

EOF
		cat >bacs2.conf <<EOF
; BACS2 Server configuration file

[General]

check_submits_delay=5000
checker_timeout=6000
compiler_timeout=10000
max_info_size=50000
max_run_out_size=64000000
max_run_idle_time=10000

default_time_limit=10
default_memory_limit=128

limit_run_exe=$dst/limit_run
limit_run_result_file=$dst/lim_run_results.txt
temp_dir=$dst/Temp
test_dir=$dst/Test
b2_compiler=$sources/b2_compiler
problem_archive_dir=$archive
default_checker=$dst/checkdef

[DB]

host=$db_host
user=$db_user
port=$db_port
pass=$db_pass
database=$db_database
reconnect=$db_reconnect

[Log]

;output = {none, console, file, both}
output=both
log_file=$dst/bacs2.log

[Lang]

(+)
name=C++ (gnu)
compile=$(which c++) -O3 -x c++ {src} -o{src}.o
exefile={src}.o
run=$dst/test_c++ {src}.o

(G)
name=GNU C++ (same)
compile=$(which c++) -O3 -x c++ {src} -o{src}.o
exefile={src}.o
run=$dst/test_c++ {src}.o

(1)
name=C++ 11
compile=$(which c++) -std=c++0x -O3 -x c++ {src} -o{src}.o
exefile={src}.o
run=$dst/test_c++ {src}.o

(C)
name=C
compile=$(which c++) -O3 -x c {src} -o{src}.o
exefile={src}.o
run=$dst/test_c++ {src}.o

(P)
name=Pascal
compile=$(which fpc) -O2 -Mdelphi {src} -o{src}.exe
tmpfile={src_noext}.o
exefile={src}.exe
run=$dst/test_c++ {src}.exe

(J)
name=Java
dir=$dst
compile={dir}/java_compile {src} Main.java
exefile={src}.dir/Main.class
no_memory_limit=1
run=$(which java) -Xmx128m -Xss128m -Djava.security.manager -Djava.security.policy={dir}/java.policy -classpath {src}.dir Main
clean={dir}/java_clean {src}

;(C)
;name=C
;dir=C:\Bacs2\VC
;compile={dir}\Bin\\cl.exe /nologo /EHsc /O2 /Ox /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"{src}.obj" /Fe"{src}.exe" /I"{dir}\Include" /Tc"{src}" /link /nologo /LIBPATH:"{dir}\Lib"
;compile={dir}\Bin\\cl.exe /nologo /O2 /Ox /Fo"{src}.obj" /Fe"{src}.exe" /I"{dir}\Include" /Tc"{src}" /link /nologo /LIBPATH:"{dir}\Lib"
;tmpfile={src}.obj
;exefile={src}.exe
;run={src}.exe

;(D)
;name=Delphi
;dir=C:\Program Files (x86)\Borland\Delphi7
;compile={dir}\Bin\dcc32.exe -Q -CC -$I+,Q-,R-,X+ -U"{dir}\Lib" "{src}"
;exefile={src_noext}.exe
;run={src_noext}.exe

;(L)
;name=Lisp
;dir=C:\Bacs2\CLisp
;compile={dir}\lisp.exe -ansi -B "{dir}\\clisp" -M "{dir}\myinit.mem" -q -q -c "{src}" -o "{src}.fas"
;exefile={src}.fas
;run={dir}\lisp.exe -ansi -B "{dir}\\clisp" -M "{dir}\myinit.mem" -q -q "{src}.fas"


;(G)
;name=GNU C++
;dir=C:\MinGW\MinGW\Bin
;compile={dir}\g++.exe -O2 -x c++ "{src}" -o "{src}.exe"
;exefile={src}.exe
;run={src}.exe

;(#)
;name=C#
;dir=C:\WINDOWS\Microsoft.NET\Framework64\v2.0.50727
;compile={dir}\csc.exe /nologo /t:exe /checked- /debug- /platform:x86 /o /out:"{src}.exe" "{src}"
;exefile={src}.exe
;run={src}.exe

EOF
	fi
done

