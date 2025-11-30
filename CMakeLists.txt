cmake_minimum_required(VERSION 3.16)
project(relight VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find framework last on macOS
set(CMAKE_FIND_FRAMEWORK LAST)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

message(STATUS "CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

# Build options
option(MACOSX_MAKE_BUNDLE "If using macOS, all targets will be placed in a relight macOS app bundle" ON)

# Qt6 detection
find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent Xml REQUIRED)
set(RELIGHT_QT Qt6)
message(STATUS "Using Qt6")



# Find dependencies
find_package(Eigen3 QUIET)
find_package(OpenMP QUIET)

# Bundle configuration for macOS
if(MACOSX_MAKE_BUNDLE)
    set(MACOSX_EXE_TARGET_OPTION MACOSX_BUNDLE)
else()
    set(MACOSX_EXE_TARGET_OPTION "")
endif()

# Eigen3 setup
if(NOT Eigen3_FOUND)
    message(STATUS "Using bundled Eigen3")
    set(EIGEN3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/eigen-3.3.9)
else()
    message(STATUS "Using system Eigen3")
endif()

# JPEG setup
if(MSVC)
    set(JPEGTURBO_HOME ${CMAKE_CURRENT_SOURCE_DIR}/external/libjpeg-turbo-2.0.6)
    set(JPEG_INCLUDE_DIR ${JPEGTURBO_HOME}/include)
    set(JPEG_LIBRARIES ${JPEGTURBO_HOME}/lib/jpeg.lib)
    message(STATUS "Using bundled libjpeg-turbo for MSVC")
else()
    find_package(JPEG REQUIRED)
    message(STATUS "Using system JPEG")
endif()

# TIFF setup
find_package(TIFF QUIET)
if(NOT TIFF_FOUND)
    message(STATUS "Using bundled TIFF library")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/tiff-4.7.0)
    add_library(TIFF::TIFF ALIAS tiff)
else()
    message(STATUS "Using system TIFF library")
endif()

# OpenCV setup, not needed for the moment,.
#if(NOT APPLE)
#    find_package(OpenCV REQUIRED)
#    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
#endif()

# LittleCMS2 setup (mandatory)
find_package(LCMS2 REQUIRED)
message(STATUS "Using LittleCMS2 include dirs: ${LCMS2_INCLUDE_DIRS}")


# Version information
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/RELIGHT_VERSION")
    # Version is read from RELIGHT_VERSION file
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/RELIGHT_VERSION" RELIGHT_VERSION)
    string(STRIP "${RELIGHT_VERSION}" RELIGHT_VERSION)
else()
    # Fallback version
    set(RELIGHT_VERSION "dev")
endif()

message(STATUS "Relight Version: ${RELIGHT_VERSION}")

# Include custom CMake tools
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/relight_tools.cmake)

# Installation options
if(WIN32 OR APPLE)
    set(INSTALL_TO_UNIX_LAYOUT OFF)
else()
    set(INSTALL_TO_UNIX_LAYOUT ON)
endif()

include(GNUInstallDirs)

# Add subdirectories
add_subdirectory(relightlab)
add_subdirectory(relight-cli)
add_subdirectory(relight-deepzoom)
add_subdirectory(relight-merge)
add_subdirectory(relight-normals)
