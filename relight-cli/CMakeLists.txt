#export CMAKE_PREFIX_PATH=path to Qt/5.5/gcc_64/ (save for bin/qmake)

cmake_minimum_required(VERSION 3.1)
project(relight-cli)

set (CMAKE_CXX_STANDARD 11)

SET(HEADERS
	rtibuilder.h
	../src/getopt.h
	../src/imageset.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/material.h
	../src/vector.h
	../src/rti.h
	../src/legacy_rti.h
)

SET(SOURCES
	main.cpp
	rtibuilder.cpp
	../src/getopt.cpp
	../src/imageset.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
	../src/rti.cpp
	../src/legacy_rti.cpp
)

cmake_minimum_required(VERSION 3.1)

project(relight-cli)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(
	Qt5
	COMPONENTS Core Gui Concurrent
	REQUIRED)
find_package(OpenMP)

if (MSVC)
	file(TO_CMAKE_PATH $ENV{JPEGTURBO_HOME} JPEGTURBO_HOME)
	file(TO_CMAKE_PATH $ENV{EIGEN_HOME} EIGEN_HOME)
	set(JPEG_INCLUDE_DIR ${JPEGTURBO_HOME}/include)
	set(JPEG_LIBRARIES ${JPEGTURBO_HOME}/lib/jpeg.lib)
	set(EIGEN3_INCLUDE_DIR ${EIGEN_HOME})
else()
	find_package (JPEG REQUIRED)
	find_package(Eigen3 REQUIRED)
endif()

add_executable(relight-cli MACOSX_BUNDLE ${HEADERS} ${SOURCES})
target_include_directories(
	relight-cli PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${EIGEN3_INCLUDE_DIR})

target_link_libraries(
	relight-cli PUBLIC
		${JPEG_LIBRARIES}
		OpenMP::OpenMP_CXX
		Qt5::Core
		Qt5::Gui
		Qt5::Concurrent)

target_compile_definitions(relight-cli PUBLIC _USE_MATH_DEFINES NOMINMAX)


INSTALL(TARGETS relight-cli
	RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)





