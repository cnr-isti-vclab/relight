cmake_minimum_required(VERSION 3.1)
project(relight)

set (CMAKE_CXX_STANDARD 11)

option(USE_QT_5 "Set to use Qt5" ON)
option(MACOSX_MAKE_BUNDLE "If using macos, all targets will be placed in a relight macosx app bundle" ON)

if (USE_QT_5)
	set(RELIGHT_QT Qt5)
else()
	set(RELIGHT_QT Qt6)
endif()

if (MACOSX_MAKE_BUNDLE)
	set(MACOSX_EXE_TARGET_OPTION MACOSX_BUNDLE)
else()
	set(MACOSX_EXE_TARGET_OPTION "")
endif()

find_package(Eigen3)
if (NOT Eigen3_FOUND)
	set(EIGEN3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/eigen-3.3.9)
endif()

if (MSVC)
	set(JPEGTURBO_HOME ${CMAKE_CURRENT_SOURCE_DIR}/external/libjpeg-turbo-2.0.6)
	set(JPEG_INCLUDE_DIR ${JPEGTURBO_HOME}/include)
	set(JPEG_LIBRARIES ${JPEGTURBO_HOME}/lib/jpeg.lib)
else()
	find_package (JPEG REQUIRED)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/RELIGHT_VERSION")
	# version is read from RELIGHT_VERSION file
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/RELIGHT_VERSION" RELIGHT_VERSION)
else() # should never happen
	set(RELIGHT_VERSION "test")
endif()

message("ReLight Version: " ${RELIGHT_VERSION})

# install options

if(WIN32 OR APPLE)
	set(INSTALL_TO_UNIX_LAYOUT OFF)
else()
	set(INSTALL_TO_UNIX_LAYOUT ON)
endif()
include(GNUInstallDirs)

add_subdirectory(relight)
add_subdirectory(relight-cli)
add_subdirectory(relight-merge)
