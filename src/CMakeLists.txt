cmake_minimum_required(VERSION 3.2)
project(WebServer)
set(CMAKE_CXX_STANDARD 14)
# 获得目录下所有的cc文件
file(GLOB_RECURSE SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/*.cc)

add_executable(WebServer main.cpp ${SOURCE_FILE})
#mysql所需的头文件和库所需的目录和库文件可以用/usr/bin/mysql_config来查看
include_directories("/usr/include/mysql")
link_directories("usr/lib/x86_64-linux-gnu")

set(MYSQL_LIBS
        mysqlclient pthread z m rt atomic ssl crypto dl
        )
target_link_libraries(WebServer ${MYSQL_LIBS})