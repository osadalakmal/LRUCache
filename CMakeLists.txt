cmake_minimum_required (VERSION 2.6)
project (lru)
add_executable(lru.tsk lru.cpp tests.cpp )

add_definitions(--std=c++0x -Wall -Wextra -Werror -g -I./)
include_directories(/usr/include/gtest)
include_directories(SYSTEM)
link_directories(/usr/local/lib)
link_directories(/usr/lib)
target_link_libraries( lru.tsk gtest pthread )

enable_testing()
add_test(A lru.tsk)
