#!/usr/bin/env bash
set -e

CFLAGS="-O2 -fno-stack-limit"
CXXFLAGS="${CFLAGS}"

thisprefix="$(dirname "$0")"
if [[ ${thisprefix:0:1} != / ]]
then
    thisprefix="$PWD/$thisprefix"
fi
if [[ -f $thisprefix/CMakeLists.txt ]]
then
    # source code
    binprefix="$thisprefix/build"
    utilprefix="$thisprefix/build"
else
    binprefix="$thisprefix"
    utilprefix="$(dirname "$thisprefix")/lib/bacs/legacy"
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
    mkdir -p "$dst"
    mkdir -p "$dst/Test" "$dst/Temp" "$dst/Archive"
    ln -sf "$binprefix/bacsd" "$dst/bacsd"
    for util in checkdef limit_run java_compile wipe java_run py3_compile.py
    do
        ln -sf "$utilprefix/$util" "$dst/$util"
    done
    cat >"$dst/java.policy" <<EOF
grant {
    permission java.io.FilePermission "*", "read,write";
};

EOF
        cat >"$dst/bacs2.conf" <<EOF
; BACS2 Server configuration file

[general]

check_submits_delay=5000
checker_timeout=6000
compiler_timeout=10000
max_info_size=50000
max_run_out_size=64000000
max_run_idle_time=10000

ping_period=10
;ping_uri=http://statistic.bacs.cs.istu.ru/Handler.ashx

bunsan_repository_config=$bunsan_pm_config
repository_prefix=bacs/problem/
repository_suffix=/legacy0

verbose_tests_copy=${verbose_tests_copy}
verbose_tests_server=${verbose_tests_server}

order=${cf_order}

default_time_limit=10
default_memory_limit=128

limit_run_exe=$dst/limit_run
limit_run_result_file=$dst/lim_run_results.txt
temp_dir=$dst/Temp
test_dir=$dst/Test
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

cat >"$dst/langs.conf" <<EOF
[+]
name=C++ (gnu)
dir=$dst
compile=$(which g++) ${CXXFLAGS} -std=c++03 -x c++ {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/wipe {dir}/Test/

[g]
name=GNU C++ (same)
dir=$dst
compile=$(which g++) ${CXXFLAGS} -std=c++03 -x c++ {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/wipe {dir}/Test/

[1]
name=C++ 11
dir=$dst
compile=$(which g++) ${CXXFLAGS} -std=c++11 -x c++ {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/wipe {dir}/Test/

[c]
name=C
dir=$dst
compile=$(which gcc) ${CFLAGS} -std=c90 -x c {src} -o{src}.o
exefile={src}.o
run={src}.o
clean=$dst/wipe {dir}/Test/

[p]
name=FPC (Delphi mode)
dir=$dst
compile=$(which fpc) -Cs\`echo '64*2^20-1025' | bc\` -Xt -O2 -Mdelphi {src} -o{src}.exe
tmpfile={src_noext}.o
exefile={src}.exe
run={src}.exe
clean=$dst/wipe {dir}/Test/

[f]
name=FPC (FPC mode)
dir=$dst
compile=$(which fpc) -Cs\`echo '64*2^20-1025' | bc\` -Xt -O2 -Mfpc {src} -o{src}.exe
tmpfile={src_noext}.o
exefile={src}.exe
run={src}.exe
clean=$dst/wipe {dir}/Test/

[j]
name=Java
dir=$dst
compile={dir}/java_compile {src} Main.java
exefile={src}.dir/Main.class
;no_memory_limit=1
run=$dst/java_run -Djava.security.manager -Djava.security.policy={dir}/java.policy -classpath {src}.dir Main
clean=$dst/wipe {src}.dir {dir}/Test/

[t]
name=Python3
dir=$dst
compile={dir}/py3_compile.py {src} {src}.py
exefile={src}.py
run=$(which python3) {src}.py
clean=$dst/wipe {dir}/Test/

[#]
name=Mono C#
dir=$dst
compile=$(which dmcs) -out:{src_noext}.exe {src}
exefile={src_noext}.exe
run=$(which mono) {src_noext}.exe

[logo-kturtle]
name=Logo
dir=$dst
compile=$(which cp) {src} {src}.turtle
exefile={src}.turtle
run=$(which bacs.kturtle) --test {src}.turtle
clean=$dst/wipe {dir}/Test/

EOF
done
