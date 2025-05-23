cmake_minimum_required(VERSION 3.13)
project(align)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()



SET(SOURCES
	main.cpp
	imagealignment.cpp
)




add_executable(align ${HEADERS} ${SOURCES})
target_include_directories(
	align PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(
	align PUBLIC
		${JPEG_LIBRARIES}
		opencv_core  opencv_imgcodecs opencv_imgproc opencv_video
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
)

target_compile_definitions(align PUBLIC _USE_MATH_DEFINES NOMINMAX)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/align/Contents/MacOS/)
	else()
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX})
	endif()
endif()

install (TARGETS align
	DESTINATION ${RELIGHT_INSTALL_BIN_DIR} 
	BUNDLE DESTINATION ${RELIGHT_INSTALL_BIN_DIR} )

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
