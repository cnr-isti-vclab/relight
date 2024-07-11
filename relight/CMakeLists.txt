cmake_minimum_required(VERSION 3.12)
project(relight)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

set (RELIGHT_HEADERS
	imagecropper.h
	mainwindow.h
	../src/sphere.h
	../src/measure.h
	graphics_view_zoom.h
	rtiexport.h
	helpdialog.h
	focaldialog.h
	settingsdialog.h
	../src/lens.h
	../src/image.h
	../src/project.h
	../src/legacy_rti.h
	../src/imageset.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/exif.h
	../src/material.h
	../src/eigenpca.h
	../src/lp.h
	../src/relight_vector.h
	../src/deepzoom.h
	../src/bni_normal_integration.h
	../relight-cli/rtibuilder.h
	processqueue.h
	queueitem.h
	queuewindow.h
	parameter.h
	task.h
	script.h
	rtitask.h
	normalstask.h
	../src/dome.h
	domecalibration.h
	qmarker.h
	qmarkerlist.h
	qmeasuremarker.h
	../src/sphere.h
	qspheremarker.h
	../src/align.h
	qalignmarker.h
	aligndialog.h
	qwhitemarker.h
	../src/white.h
	history.h
	convertdialog.h
	zoom.h
	zoomtask.h
	zoomdialog.h
	dstretch.h
	dstretchtask.h
	dstretchdialog.h
)

set (RELIGHT_SOURCES
	main.cpp
	imagecropper.cpp
	mainwindow.cpp
	../src/sphere.cpp
	../src/measure.cpp
	graphics_view_zoom.cpp
	rtiexport.cpp
	httpserver.cpp
	../src/lens.cpp
	../src/image.cpp
	../src/project.cpp
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
	../src/deepzoom.cpp
	../src/bni_normal_integration.cpp
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
	normalstask.cpp
	domecalibration.cpp
	../src/dome.cpp
	qmarkerlist.cpp
	qmarker.cpp
	qmeasuremarker.cpp
	../src/sphere.cpp
	qspheremarker.cpp
	../src/align.cpp
	qalignmarker.cpp
	qwhitemarker.cpp
	../src/white.cpp
	history.cpp
	convertdialog.cpp
	zoomtask.cpp
	zoomdialog.cpp
	dstretchtask.cpp
	dstretchdialog.cpp
)

set (RELIGHT_FORMS
	mainwindow.ui
	rtiexport.ui
	helpdialog.ui
	queuewindow.ui
	settingsdialog.ui
	focaldialog.ui
	domecalibration.ui
	convertdialog.ui
	dstretchdialog.ui
)

set (RELIGHT_RESOURCES
	icons.qrc
)

add_executable(relight ${MACOSX_EXE_TARGET_OPTION} ${RELIGHT_HEADERS} ${RELIGHT_SOURCES} ${RELIGHT_FORMS} ${RELIGHT_RESOURCES})
target_include_directories(
	relight PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${EIGEN3_INCLUDE_DIR}
	)
	
target_link_libraries(
	relight PUBLIC
		${JPEG_LIBRARIES}
		OpenMP::OpenMP_CXX
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
		${RELIGHT_QT}::Widgets
		${RELIGHT_QT}::Concurrent
		${RELIGHT_QT}::Xml
	)

target_compile_definitions(relight PUBLIC _USE_MATH_DEFINES NOMINMAX)

target_compile_definitions(relight
	PUBLIC
		RELIGHT_VERSION=${RELIGHT_VERSION})

if (APPLE)
	set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../build_scripts/relight.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../build_scripts/relight.icns DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/relight.app/Contents/Resources/")
	set_target_properties(relight PROPERTIES 
		MACOSX_BUNDLE_ICON_FILE relight.icns
		MACOSX_BUNDLE_BUNDLE_VERSION "${RELIGHT_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELIGHT_VERSION}"
		MACOSX_BUNDLE_INFO_STRING "Relight ${RELIGHT_VERSION}"
		MACOSX_BUNDLE_COPYRIGHT "Copyright VCG-ISTI-CNR © 2005-2023. All rights reserved."
		)
	
	set_additional_settings_info_plist(
		TARGET relight
		FILE ${CMAKE_CURRENT_BINARY_DIR}/relight.app/Contents/Info.plist)
endif()

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	set(RELIGHT_INSTALL_BIN_DIR .)
endif()

install (TARGETS relight DESTINATION ${RELIGHT_INSTALL_BIN_DIR})

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION .)
endif()
