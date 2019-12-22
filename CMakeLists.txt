cmake_minimum_required(VERSION 3.10)

project(sqlite-pp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(sqlite sqlite.cpp
                      pager.cpp)