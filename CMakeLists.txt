cmake_minimum_required(VERSION 3.2)
project(WebServer)
set(CMAKE_CXX_STANDARD 14)

# 设置可执行文件和库文件输出的目录
set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/src)