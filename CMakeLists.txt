project(bacs)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(YandexContestCommon)

# BOOST
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS system filesystem serialization unit_test_framework program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND libraries ${Boost_LIBRARIES})

# MySQL
find_path(mysql_include_dir NAMES mysql.h PATHS /usr/include /usr/include/mysql /usr/local/include /usr/local/include/mysql)
find_path(mysql_link_dir NAMES libmysqlclient.so PATHS /usr/lib /usr/lib/mysql /usr/local/lib /usr/local/lib/mysql)

message(STATUS "mysql.h was found in ${mysql_include_dir}")
message(STATUS "libmysqlclient.so was found in ${mysql_include_dir}")

include_directories(${mysql_include_dir})
link_directories(${mysql_link_dir})

list(APPEND libraries mysqlclient)

# bunsan
list(APPEND libraries bunsan_pm_compatibility)

# rt
list(APPEND libraries rt)

# Local
include_directories(include/bacs)

# Targets
add_library(${PROJECT_NAME} SHARED
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
target_link_libraries(${PROJECT_NAME} ${libraries})

add_executable(${PROJECT_NAME}d src/bin/bacsd.cpp)
target_link_libraries(${PROJECT_NAME}d ${PROJECT_NAME} ${libraries})

add_executable(checkdef src/bin/checkdef.cpp)
add_executable(limit_run src/bin/limit_run.cpp)
target_link_libraries(limit_run rt)
add_executable(java_compile src/bin/java_compile.c)
add_executable(clean src/bin/clean.c)
add_executable(java_run src/bin/java_run.c)
