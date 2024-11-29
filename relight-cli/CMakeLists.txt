cmake_minimum_required(VERSION 3.13)
project(relight-cli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
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
	../src/dome.h
	../src/sphere.h
	../src/lens.h
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
	../src/dome.cpp
	../src/lens.cpp
	../src/sphere.cpp
	convert_rti.cpp
)



add_executable(relight-cli ${HEADERS} ${SOURCES})
target_include_directories(
	relight-cli PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${EIGEN3_INCLUDE_DIR})

target_link_libraries(
	relight-cli PUBLIC
		${JPEG_LIBRARIES}
		OpenMP::OpenMP_CXX
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
		${RELIGHT_QT}::Concurrent)

target_compile_definitions(relight-cli PUBLIC _USE_MATH_DEFINES NOMINMAX)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/relight.app/Contents/MacOS/)
	else()
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX})
	endif()
endif()

install (TARGETS relight-cli 
	DESTINATION ${RELIGHT_INSTALL_BIN_DIR} 
	BUNDLE DESTINATION ${RELIGHT_INSTALL_BIN_DIR} )

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
