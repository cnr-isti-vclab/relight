cmake_minimum_required(VERSION 3.13)
project(relight-merge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

SET(HEADERS
	../src/getopt.h
	../src/rti.h
	../src/jpeg_decoder.h
	../src/imageset.h
	../src/lp.h
	../src/jpeg_encoder.h
	../src/lens.h
	../src/dome.h
	../src/sphere.h
	../src/crop.h
	../src/cli/rtibuilder.h
)

SET(SOURCES
	main.cpp
	../src/getopt.cpp
	../src/rti.cpp
	../src/jpeg_decoder.cpp
	../src/imageset.cpp
	../src/lp.cpp
	../src/jpeg_encoder.cpp
	../src/lens.cpp
	../src/dome.cpp
	../src/sphere.cpp
	../src/crop.cpp
	../src/cli/rtibuilder.cpp
)

add_executable(relight-merge ${HEADERS} ${SOURCES})
target_include_directories(
	relight-merge PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR})

if(Eigen3_FOUND)
	target_link_libraries(relight-merge PUBLIC Eigen3::Eigen)
else()
	target_include_directories(relight-merge PUBLIC ${EIGEN3_INCLUDE_DIR})
endif()		

target_link_libraries(
	relight-merge PUBLIC
		${JPEG_LIBRARIES}
		OpenMP::OpenMP_CXX
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
		${RELIGHT_QT}::Concurrent)

target_compile_definitions(relight-merge PUBLIC _USE_MATH_DEFINES NOMINMAX)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/relight.app/Contents/MacOS/)
	else()
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX})
	endif()
endif()

install (TARGETS relight-merge 
	DESTINATION ${RELIGHT_INSTALL_BIN_DIR})

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
