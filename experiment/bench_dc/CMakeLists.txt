cmake_minimum_required(VERSION 3.10)
project(bench)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

add_executable(bench_start
     main.cpp 
     tree/tree_log.h 
     tree/tree_generator.h 
     tree/tree_log.cpp 
     constants.h 
     util.h 
     exp_runner.h 
     tree/tree_generator.cpp 
     tree/tree_cmd.cpp 
     tree/tree_cmd.h 
     tree/tree_basics.h 
     util.cpp 
     )

target_link_libraries(bench_start hiredis)
