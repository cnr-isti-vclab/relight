cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(ASSM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Eigen3)
if(NOT Eigen3_FOUND)
	set(EIGEN3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../eigen-3.3.9)
	
endif()

include_directories(${EIGEN3_INCLUDE_DIR})

file(GLOB SOURCES ./*.cpp ./algorithms/*.cpp)
file(GLOB HEADERS ./*.h ./algorithms/*.h)

add_library(assm ${SOURCES} ${HEADERS})

