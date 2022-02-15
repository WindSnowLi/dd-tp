cmake_minimum_required(VERSION 3.20)
project(dd-tp)

set(CMAKE_CXX_STANDARD 17)
file(GLOB source_files "*.hpp")

add_executable(tp ${source_files} main.cpp)
