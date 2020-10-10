cmake_minimum_required(VERSION 3.0)
project(main)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -std=c++11 -pthread")
set(SOURCE main.cpp uid.h tree.h)
#set(SOURCE main.cpp uid.h tree.h)
#set(SOURCE test.cpp)
include_directories(generator)
link_directories(/usr/local/lib/)

add_executable(test ${SOURCE})

target_link_libraries(test -lhiredis)