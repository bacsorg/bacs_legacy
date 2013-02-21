#!/usr/bin/env bash
set -e

CFLAGS="-O2 -fno-stack-limit"
CXXFLAGS="${CFLAGS}"

sources="$(dirname "$0")"

if [[ ${sources:0:1} != / ]]
then
	sources="$PWD/$sources"
fi

# default config
cf_compile_checkers=1
cf_check_solutions=1
# end of default config

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
	if false #[ -e "$dst" ]
	then
		echo "Remove \"$dst\" if you want to init environment in it" >&2
	else
		mkdir -p "$dst"
		cd "$dst"
		cmake "$sources"
		make -j6
		mkdir -p Test Temp Archive
		cat >java.policy <<EOF
grant {
	permission java.io.FilePermission "*", "read,write";
};

EOF
		cat >bacs2.conf <<EOF
; BACS2 Server configuration file

[general]

check_submits_delay=5000
checker_timeout=6000
compiler_timeout=10000
max_info_size=50000
max_run_out_size=64000000
max_run_idle_time=10000

ping_period=10
ping_uri=http://statistic.bacs.cs.istu.ru/Handler.ashx

bunsan_repository_config=$bunsan_pm_config
repository_prefix=bacs/problem/
repository_suffix=/legacy0

order=${cf_order}

compile_checkers=$cf_compile_checkers
check_solutions=$cf_check_solutions

default_time_limit=10
default_memory_limit=128

limit_run_exe=$dst/limit_run
limit_run_result_file=$dst/lim_run_results.txt
temp_dir=$dst/Temp
test_dir=$dst/Test
b2_compiler=$sources/b2_compiler
problem_archive_dir=$dst/Archive
default_checker=$dst/checkdef

uid=$UID
gid=$GID

[db]

host=$db_host
user=$db_user
port=$db_port
pass=$db_pass
database=$db_database
reconnect=$db_reconnect

[log]

;output = {none, console, file, both}
output=both
log_file=$dst/bacs2.log

[langs]
config = $dst/langs.conf

EOF

cat >"langs.conf" <<EOF
[+]
name=C++ (gnu)
dir=$dst
compile=$(which c++) ${CXXFLAGS} -x c++ {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/clean {dir}/Test/

[g]
name=GNU C++ (same)
dir=$dst
compile=$(which c++) ${CXXFLAGS} -x c++ {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/clean {dir}/Test/

[1]
name=C++ 11
dir=$dst
compile=$(which c++) ${CXXFLAGS} -std=c++0x -x c++ {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/clean {dir}/Test/

[c]
name=C
dir=$dst
compile=$(which c++) ${CXXFLAGS} -x c {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/clean {dir}/Test/

[p]
name=FPC (Delphi mode)
dir=$dst
compile=$(which fpc) -Cs\`echo '64*2^20-1025' | bc\` -Xt -O2 -Mdelphi {src} -o{src}.exe
tmpfile={src_noext}.o
exefile={src}.exe
run={src}.exe
clean=$dst/clean {dir}/Test/

[f]
name=FPC (FPC mode)
dir=$dst
compile=$(which fpc) -Cs\`echo '64*2^20-1025' | bc\` -Xt -O2 -Mfpc {src} -o{src}.exe
tmpfile={src_noext}.o
exefile={src}.exe
run={src}.exe
clean=$dst/clean {dir}/Test/

[j]
name=Java
dir=$dst
compile={dir}/java_compile {src} Main.java
exefile={src}.dir/Main.class
;no_memory_limit=1
run=$dst/java_run -Djava.security.manager -Djava.security.policy={dir}/java.policy -classpath {src}.dir Main
clean=$dst/clean {src}.dir {dir}/Test/

[t]
name=Python3
dir=$dst
compile=$sources/src/bin/py3_compile.py {src} {src}.py
exefile={src}.py
run=$(which python3) {src}.py
clean=$dst/clean {dir}/Test/

[#]
name=Mono C#
dir=$dst
compile=$(which mcs) {src} -o {src_noext}.exe
exefile={src_noext}.exe
run=$(which mono) {src_noext}.exe

;[c]
;name=C
;dir=C:\Bacs2\VC
;compile={dir}\Bin\\cl.exe /nologo /EHsc /O2 /Ox /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"{src}.obj" /Fe"{src}.exe" /I"{dir}\Include" /Tc"{src}" /link /nologo /LIBPATH:"{dir}\Lib"
;compile={dir}\Bin\\cl.exe /nologo /O2 /Ox /Fo"{src}.obj" /Fe"{src}.exe" /I"{dir}\Include" /Tc"{src}" /link /nologo /LIBPATH:"{dir}\Lib"
;tmpfile={src}.obj
;exefile={src}.exe
;run={src}.exe

;[d]
;name=Delphi
;dir=C:\Program Files (x86)\Borland\Delphi7
;compile={dir}\Bin\dcc32.exe -Q -CC -$I+,Q-,R-,X+ -U"{dir}\Lib" "{src}"
;exefile={src_noext}.exe
;run={src_noext}.exe

;[l]
;name=Lisp
;dir=C:\Bacs2\CLisp
;compile={dir}\lisp.exe -ansi -B "{dir}\\clisp" -M "{dir}\myinit.mem" -q -q -c "{src}" -o "{src}.fas"
;exefile={src}.fas
;run={dir}\lisp.exe -ansi -B "{dir}\\clisp" -M "{dir}\myinit.mem" -q -q "{src}.fas"


;[g]
;name=GNU C++
;dir=C:\MinGW\MinGW\Bin
;compile={dir}\g++.exe -O2 -x c++ "{src}" -o "{src}.exe"
;exefile={src}.exe
;run={src}.exe

;[#]
;name=C#
;dir=C:\WINDOWS\Microsoft.NET\Framework64\v2.0.50727
;compile={dir}\csc.exe /nologo /t:exe /checked- /debug- /platform:x86 /o /out:"{src}.exe" "{src}"
;exefile={src}.exe
;run={src}.exe

EOF
	cd ..
	fi
done

