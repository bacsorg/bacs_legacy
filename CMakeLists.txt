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

add_executable(${PROJECT_NAME} ${bacs_srcs})
target_link_libraries(${PROJECT_NAME} mysqlclient)

add_executable(checkdef checkdef.cpp)
add_executable(limit_run limit_run.cpp)
add_executable(java_compile java_compile.cpp)
add_executable(java_clean java_clean.cpp)

set(b2_srcs ${bacs_srcs})
list(REMOVE_ITEM b2_srcs ${CMAKE_SOURCE_DIR}/bacs2.cpp)
message("${b2_srcs}")
add_executable(b2_compiler b2_compiler.cpp ${b2_srcs})
target_link_libraries(b2_compiler mysqlclient)

