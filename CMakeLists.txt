project(bacs2)

cmake_minimum_required(VERSION 2.8)

file(GLOB bacs_srcs bacs2*.cpp)

find_path(mysql_include_dir NAMES mysql.h PATHS /usr/include/mysql /usr/local/include/mysql)

message(STATUS "mysql.h was found in ${mysql_include_dir}")

include_directories(${mysql_include_dir})

add_executable(${PROJECT_NAME} ${bacs_srcs})
target_link_libraries(${PROJECT_NAME} mysqlclient)

add_executable(limit_run main.cpp)

