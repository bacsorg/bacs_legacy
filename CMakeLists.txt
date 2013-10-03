project(bacs)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(BunsanCommon)

bunsan_use_boost(system filesystem)

# MySQL
find_path(mysql_include_dir NAMES mysql.h PATHS /usr/include /usr/include/mysql /usr/local/include /usr/local/include/mysql)
find_path(mysql_link_dir NAMES libmysqlclient.so PATHS /usr/lib /usr/lib/mysql /usr/local/lib /usr/local/lib/mysql)
message(STATUS "mysql.h was found in ${mysql_include_dir}")
message(STATUS "libmysqlclient.so was found in ${mysql_include_dir}")
include_directories(${mysql_include_dir})
link_directories(${mysql_link_dir})
bunsan_use(mysqlclient)

bunsan_use_bunsan(pm_compatibility)

# Local
include_directories(include/bacs)

# Targets
bunsan_add_shared_library(${PROJECT_NAME}
    src/lib/helper.cpp
    src/lib/test.cpp
    src/lib/db.cpp
    src/lib/run.cpp
    src/lib/tempfiles.cpp
    src/lib/log.cpp
    src/lib/util.cpp
    src/lib/common.cpp
    src/lib/pipes.cpp
    src/lib/problem.cpp
    src/lib/config.cpp
    src/lib/submit.cpp
)
bunsan_use(${PROJECT_NAME})

bunsan_add_executable(${PROJECT_NAME}d src/bin/bacsd.cpp)

add_executable(checkdef src/bin/checkdef.cpp)
add_executable(limit_run src/bin/limit_run.cpp)
target_link_libraries(limit_run rt)
add_executable(java_compile src/bin/java_compile.c)
add_executable(clean src/bin/clean.c)
add_executable(java_run src/bin/java_run.c)
