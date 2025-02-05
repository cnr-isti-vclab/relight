cmake_minimum_required(VERSION 3.13)
project(relight)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_FIND_FRAMEWORK LAST)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

message(CMAKE_MODULE_PATH="${CMAKE_MODULE_PATH}")

option(MACOSX_MAKE_BUNDLE "If using macos, all targets will be placed in a relight macosx app bundle" ON)


find_package(Qt6 COMPONENTS  Core Gui Widgets Concurrent Xml QUIET)


if (Qt6_FOUND)
	set(RELIGHT_QT Qt6)
	message("Using Qt6")

else()
	find_package(Qt5 COMPONENTS  Core Gui Widgets Concurrent Xml QUIET)
	
	if(Qt5_FOUND)
		set(RELIGHT_QT Qt5)
		message("Using Qt5")
	else()
		message(FATAL_ERROR "Qt not found")
	endif()
endif()


find_package(Eigen3)
find_package(OpenMP)

if(MACOSX_MAKE_BUNDLE)
	set(MACOSX_EXE_TARGET_OPTION MACOSX_BUNDLE)
else()
	set(MACOSX_EXE_TARGET_OPTION "")
endif()

if(NOT Eigen3_FOUND)
	set(EIGEN3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/eigen-3.3.9)
endif()

if(MSVC)
	set(JPEGTURBO_HOME ${CMAKE_CURRENT_SOURCE_DIR}/external/libjpeg-turbo-2.0.6)
	set(JPEG_INCLUDE_DIR ${JPEGTURBO_HOME}/include)
	set(JPEG_LIBRARIES ${JPEGTURBO_HOME}/lib/jpeg.lib)	
else()
	find_package (JPEG REQUIRED)
endif()
	
find_package (TIFF)
if(NOT TIFF_FOUND)
	add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/external/tiff-4.7.0)
	add_library(TIFF::TIFF ALIAS tiff)
endif()

find_package(OpenCV REQUIRED)
#message(STATUS ${OpenCV_INCLUDE_DIRS})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/RELIGHT_VERSION")
	# version is read from RELIGHT_VERSION file
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/RELIGHT_VERSION" RELIGHT_VERSION)
else() # should never happen
	set(RELIGHT_VERSION "test")
endif()

message("Relight Version: " ${RELIGHT_VERSION})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/relight_tools.cmake)

# install options

if(WIN32 OR APPLE)
	set(INSTALL_TO_UNIX_LAYOUT OFF)
else()
	set(INSTALL_TO_UNIX_LAYOUT ON)
endif()
include(GNUInstallDirs)

add_subdirectory(relight)
add_subdirectory(relightlab)
add_subdirectory(relight-cli)
add_subdirectory(relight-merge)
add_subdirectory(align)
