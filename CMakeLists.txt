project(bacs2)

cmake_minimum_required(VERSION 2.8)

if(${CMAKE_COMPILER_IS_GNUCXX})
    set(gxx_flags "-std=c++0x -Wall -Wextra")
    if(${UNIX})
        set(gxx_flags "-pthread ${gxx_flags}")
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG "-g ${gxx_flags}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${gxx_flags}")
endif()

include(${CMAKE_SOURCE_DIR}/user-config.cmake OPTIONAL)

# BOOST
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS system filesystem serialization iostreams unit_test_framework program_options REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

file(GLOB bacs_srcs bacs2*.cpp)

find_path(mysql_include_dir NAMES mysql.h PATHS /usr/include /usr/include/mysql /usr/local/include /usr/local/include/mysql)
find_path(mysql_link_dir NAMES libmysqlclient.so PATHS /usr/lib /usr/lib/mysql /usr/local/lib /usr/local/lib/mysql)

message(STATUS "mysql.h was found in ${mysql_include_dir}")
message(STATUS "libmysqlclient.so was found in ${mysql_include_dir}")

add_definitions(-g -Wall -Wextra)

include_directories(${mysql_include_dir})
link_directories(${mysql_link_dir})

add_executable(${PROJECT_NAME} main.cpp ${bacs_srcs})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} mysqlclient rt bunsan_pm_compatibility)

add_executable(checkdef checkdef.cpp)
add_executable(limit_run limit_run.cpp)
target_link_libraries(limit_run rt)
add_executable(java_compile java_compile.c)
add_executable(clean clean.c)
add_executable(java_run java_run.c)

