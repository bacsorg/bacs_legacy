project(bacs2)

cmake_minimum_required(VERSION 2.8)

file(GLOB bacs_srcs bacs2*.cpp)

find_path(mysql_include_dir NAMES mysql.h PATHS /usr/include /usr/include/mysql /usr/local/include /usr/local/include/mysql)
find_path(mysql_link_dir NAMES libmysqlclient.so PATHS /usr/lib /usr/lib/mysql /usr/local/lib /usr/local/lib/mysql)

message(STATUS "mysql.h was found in ${mysql_include_dir}")
message(STATUS "libmysqlclient.so was found in ${mysql_include_dir}")

add_definitions(-g -Wall -Wextra)

include_directories(${mysql_include_dir})
link_directories(${mysql_link_dir})

add_executable(${PROJECT_NAME} main.cpp ${bacs_srcs})
target_link_libraries(${PROJECT_NAME} mysqlclient rt bunsan_pm_compatibility)

add_executable(checkdef checkdef.cpp)
add_executable(limit_run limit_run.cpp)
target_link_libraries(limit_run rt)
add_executable(java_compile java_compile.c)
add_executable(clean clean.c)

