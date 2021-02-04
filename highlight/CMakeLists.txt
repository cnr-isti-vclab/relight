cmake_minimum_required(VERSION 3.12)
project(relight)

set(CMAKE_CXX_STANDARD 17)

find_package(
	Qt5
	COMPONENTS Core Gui Widgets Concurrent
	REQUIRED)
if (MSVC)
	set(JPEG_INCLUDE_DIR $ENV{JPEGTURBO_HOME}/include)
	set(JPEG_LIBRARIES $ENV{JPEGTURBO_HOME}/lib/jpeg.lib)
	set(EIGEN3_INCLUDE_DIR $ENV{EIGEN_HOME})
	
	message("jlibs: " ${JPEG_LIBRARIES})
	message("jinc: " ${JPEG_INCLUDE_DIR})
	message("eiging: " ${EIGEN3_INCLUDE_DIR})
else()
	find_package (JPEG REQUIRED)
	find_package(Eigen3 REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set (RELIGHT_HEADERS
	imagecropper.h
	imagecropper_e.h
	imagecropper_p.h
	mainwindow.h
	ball.h
	graphics_view_zoom.h
	rtiexport.h
	helpdialog.h
	../src/imageset.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/material.h
	../src/eigenpca.h
	../relight/rtibuilder.h
)

set (RELIGHT_SOURCES
	main.cpp
	imagecropper.cpp
	mainwindow.cpp
	ball.cpp
	graphics_view_zoom.cpp
	rtiexport.cpp
	../relight/relight.cpp
	../src/imageset.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
	../src/material.cpp
	../src/rti.cpp
	helpdialog.cpp
)

set (RELIGHT_FORMS
	mainwindow.ui
	rtiexport.ui
	helpdialog.ui
)

set (RELIGHT_RESOURCES
	icons.qrc
)

add_executable(relight ${RELIGHT_HEADERS} ${RELIGHT_SOURCES} ${RELIGHT_FORMS} ${RELIGHT_RESOURCES})
target_include_directories(
	relight PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${EIGEN3_INCLUDE_DIR})
target_link_libraries(
	relight PUBLIC
		${JPEG_LIBRARIES}
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
		Qt5::Concurrent)
target_compile_definitions(relight PUBLIC _USE_MATH_DEFINES NOMINMAX)
