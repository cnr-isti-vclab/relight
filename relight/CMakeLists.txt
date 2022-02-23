cmake_minimum_required(VERSION 3.12)
project(relight)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

find_package(
	Qt5
	COMPONENTS Core Gui Widgets Concurrent
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
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

set (RELIGHT_HEADERS
	imagecropper.h
	mainwindow.h
	sphere.h
	measure.h
	graphics_view_zoom.h
	rtiexport.h
	helpdialog.h
	focaldialog.h
	settingsdialog.h
	lens.h
	image.h
	project.h
	../src/legacy_rti.h
	../src/imageset.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/exif.h
	../src/material.h
	../src/eigenpca.h
	../src/lp.h
	../src/vector.h
	../relight-cli/rtibuilder.h
	processqueue.h
	queueitem.h
	queuewindow.h
	parameter.h
	task.h
	script.h
	rtitask.h
	dome.h
	domecalibration.h
	qmarker.h
	qmarkerlist.h
	qmeasuremarker.h
	sphere.h
	qspheremarker.h
	align.h
	qalignmarker.h
	aligndialog.h
	qwhitemarker.h
	white.h
	convertdialog.h
)

set (RELIGHT_SOURCES
	main.cpp
	imagecropper.cpp
	mainwindow.cpp
	sphere.cpp
	measure.cpp
	graphics_view_zoom.cpp
	rtiexport.cpp
	httpserver.cpp
	lens.cpp
	image.cpp
	project.cpp
	scripts.cpp
	../relight-cli/rtibuilder.cpp
	../relight-cli/convert_rti.cpp
	../src/imageset.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
	../src/rti.cpp
	../src/legacy_rti.cpp
	../src/exif.cpp
	../src/lp.cpp
	helpdialog.cpp
	focaldialog.cpp
	settingsdialog.cpp
	aligndialog.cpp
	processqueue.cpp
	queueitem.cpp
	queuewindow.cpp
	parameter.cpp
	task.cpp
	script.cpp
	rtitask.cpp
	domecalibration.cpp
	dome.cpp
	qmarkerlist.cpp
	qmarker.cpp
	qmeasuremarker.cpp
	sphere.cpp
	qspheremarker.cpp
	align.cpp
	qalignmarker.cpp
	qwhitemarker.cpp
	white.cpp
	convertdialog.cpp
)

set (RELIGHT_FORMS
	mainwindow.ui
	rtiexport.ui
	helpdialog.ui
	queuewindow.ui
	settingsdialog.ui
	focaldialog.ui
	domecalibration.ui
	convertdialog
)

set (RELIGHT_RESOURCES
	icons.qrc
)

add_executable(relight MACOSX_BUNDLE ${RELIGHT_HEADERS} ${RELIGHT_SOURCES} ${RELIGHT_FORMS} ${RELIGHT_RESOURCES})
target_include_directories(
	relight PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${EIGEN3_INCLUDE_DIR})
		
target_link_libraries(
	relight PUBLIC
		${JPEG_LIBRARIES}
		OpenMP::OpenMP_CXX
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
		Qt5::Concurrent)
		
target_compile_definitions(relight PUBLIC _USE_MATH_DEFINES NOMINMAX)

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

install (TARGETS relight DESTINATION ${RELIGHT_INSTALL_BIN_DIR})

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION .)
endif()
