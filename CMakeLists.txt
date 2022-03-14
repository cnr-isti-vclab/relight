#export CMAKE_PREFIX_PATH=path to Qt/5.5/gcc_64/ (where you would find bin/qmake)
cmake_minimum_required(VERSION 3.1)
project(relight)

set (CMAKE_CXX_STANDARD 11)

add_subdirectory(relight)
add_subdirectory(relight-cli)
add_subdirectory(relight-merge)
