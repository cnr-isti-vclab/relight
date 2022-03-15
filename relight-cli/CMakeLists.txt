cmake_minimum_required(VERSION 3.12)
project(relight-cli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

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

set(CMAKE_AUTOMOC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()




SET(HEADERS
	rtibuilder.h
	../src/getopt.h
	../src/imageset.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/material.h
	../src/relight_vector.h
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
	../src/lp.cpp
	convert_rti.cpp
)



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

if(WIN32 OR APPLE)
	set(INSTALL_TO_UNIX_LAYOUT OFF)
else()
	set(INSTALL_TO_UNIX_LAYOUT ON)
endif()
include(GNUInstallDirs)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	set(RELIGHT_INSTALL_BIN_DIR .)
endif()

install (TARGETS relight-cli 
	DESTINATION ${RELIGHT_INSTALL_BIN_DIR} 
	BUNDLE DESTINATION ${RELIGHT_INSTALL_BIN_DIR} )

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION .)
endif()
