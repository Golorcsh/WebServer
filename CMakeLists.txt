cmake_minimum_required(VERSION 3.2)
project(WebServer)
set(CMAKE_CXX_STANDARD 14)
# 获得目录下所有的cc文件
file(GLOB_RECURSE SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/*.cc)
add_executable(WebServer main.cpp ${SOURCE_FILE})