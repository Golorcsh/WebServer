cmake_minimum_required(VERSION 3.22)
project(WebServer)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(src SRC)
add_executable(WebServer  main.cpp ${SRC})