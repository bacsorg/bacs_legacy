project(bacs2)

cmake_minimum_required(VERSION 2.8)

file(GLOB bacs_srcs bacs2*.cpp)

include_directories(/usr/include/mysql)

add_executable(${PROJECT_NAME} ${bacs_srcs})
target_link_libraries(${PROJECT_NAME} mysqlclient)

add_executable(limit_run main.cpp)

