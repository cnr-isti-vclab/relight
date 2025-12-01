cmake_minimum_required(VERSION 3.13)
project(relight-deepzoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

SET(HEADERS
	../src/deepzoom.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
)

SET(SOURCES
	main.cpp
	../src/deepzoom.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
)

set(RESOURCES
	../src/icc_profiles.qrc
)

add_executable(relight-deepzoom ${HEADERS} ${SOURCES} ${RESOURCES})
target_include_directories(
	relight-deepzoom PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		TIFF::TIFF)

target_link_libraries(
	relight-deepzoom PUBLIC
		${JPEG_LIBRARIES}
		TIFF::TIFF
		${RELIGHT_QT}::Core)

target_compile_definitions(relight-deepzoom PUBLIC _USE_MATH_DEFINES NOMINMAX)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/relightlab.app/Contents/MacOS/)
	else()
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX})
	endif()
endif()

install (TARGETS relight-deepzoom 
	DESTINATION ${RELIGHT_INSTALL_BIN_DIR})

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
