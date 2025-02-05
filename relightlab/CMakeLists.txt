cmake_minimum_required(VERSION 3.12)
project(relightlab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

find_package(
	${RELIGHT_QT}
	COMPONENTS Core Gui Widgets Concurrent Xml
	REQUIRED)
find_package(OpenMP)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()



set (RELIGHT_HEADERS
    processqueue.h 
    ../src/align.h 
    ../src/dome.h 
    ../src/exif.h 
    ../src/image.h 
    ../src/lens.h 
    ../src/lp.h 
    ../src/measure.h 
    ../src/project.h 
    ../src/sphere.h 
    ../src/white.h
    ../src/bni_normal_integration.h
    ../src/fft_normal_integration.h
    alignrow.h
    alignframe.h
    alignframe.h
    canvas.h 
    lightgeometry.h 
    mainwindow.h 
    mainwindow.h 
    preferences.h 
    recentprojects.h 
    reflectionview.h 
    relightapp.h 
    sphererow.h 
    tabwidget.h 
    imageframe.h 
    imageview.h
    homeframe.h 
    imagelist.h 
    flowlayout.h 
    imagegrid.h 
    lightsframe.h 
    markerdialog.h
    domepanel.h 
    directionsview.h 
    rtiexportdialog.h
    spherepanel.h 
    spherepicking.h 
    spheredialog.h 
    task.h 
    verifyview.h 
    verifydialog.h 
    helpbutton.h 
    cropframe.h 
    rtiframe.h
    rticard.h
    rtitask.h
    ../relight/imagecropper.h
    ../relight-cli/rtibuilder.h
    ../relight/zoom.h  
    ../src/deepzoom.h
    ../src/rti.h
    ../src/legacy_rti.h
    ../src/imageset.h
    ../src/jpeg_encoder.h
    ../src/jpeg_decoder.h
    ../src/flatnormals.h
    ../relight/httpserver.h
    queueframe.h
    queueitem.h
    normalsframe.h
    normalstask.h
    normalsplan.h
    planrow.h
    scaleframe.h
)

set (RELIGHTLAB_SOURCES
	main.cpp 
    processqueue.cpp 
    ../src/align.cpp 
    ../src/dome.cpp 
    ../src/exif.cpp 
    ../src/image.cpp 
    ../src/lens.cpp 
    ../src/lp.cpp 
    ../src/measure.cpp 
    ../src/project.cpp 
    ../src/sphere.cpp 
    ../src/white.cpp 
    ../src/bni_normal_integration.cpp
    ../src/fft_normal_integration.cpp
    alignframe.cpp
    alignpicking.cpp
    alignrow.cpp
    canvas.cpp 
    domepanel.cpp 
    lightgeometry.cpp 
    mainwindow.cpp 
    preferences.cpp 
    recentprojects.cpp 
    reflectionview.cpp 
    relightapp.cpp 
    sphererow.cpp 
    tabwidget.cpp 
    imageframe.cpp
    imageview.cpp 
    homeframe.cpp 
    imagelist.cpp 
    flowlayout.cpp 
    imagegrid.cpp 
    lightsframe.cpp 
    markerdialog.cpp
    directionsview.cpp 
    spherepanel.cpp 
    spherepicking.cpp 
    spheredialog.cpp 
    task.cpp 
    verifyview.cpp 
    verifydialog.cpp 
    helpbutton.cpp 
    cropframe.cpp
    rtiframe.cpp
    rtirecents.cpp
    rtiplan.cpp
    rticard.cpp
    rtiexportdialog.cpp
    rtitask.cpp
    ../relight/imagecropper.cpp
    ../relight-cli/rtibuilder.cpp
    ../src/deepzoom.cpp
    ../src/rti.cpp
    ../src/legacy_rti.cpp
    ../src/imageset.cpp
    ../src/jpeg_encoder.cpp
    ../src/jpeg_decoder.cpp
    ../src/flatnormals.cpp
    ../relight/httpserver.cpp
    queueframe.cpp
    queueitem.cpp
    normalsframe.cpp
    normalstask.cpp
    normalsplan.cpp
    planrow.cpp
    scaleframe.cpp
)


set (RELIGHTLAB_RESOURCES
	res.qrc
)

file(GLOB ASSM_SOURCES ../external/assm/SurfaceMesh.cpp ../external/assm/algorithms/*.cpp)
file(GLOB ASSM_HEADERS ../external/assm/*.h ../external/assm/algorithms/*.h)


add_executable(relightlab ${MACOSX_EXE_TARGET_OPTION} ${RELIGHTLAB_HEADERS} ${RELIGHTLAB_SOURCES} ${ASSM_SOURCES} ${ASSM_HEADERS} ${RELIGHTLAB_RESOURCES})
target_include_directories(
	relightlab PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		TIFF::TIFF
		${EIGEN3_INCLUDE_DIR}
		../external
	)
	
target_link_libraries(
	relightlab PUBLIC
		${JPEG_LIBRARIES}
		TIFF::TIFF
		OpenMP::OpenMP_CXX
		opencv_core  opencv_imgcodecs opencv_imgproc opencv_video
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
		${RELIGHT_QT}::Widgets
		${RELIGHT_QT}::Concurrent
		${RELIGHT_QT}::Xml
	)

target_compile_definitions(relightlab PUBLIC _USE_MATH_DEFINES NOMINMAX)

target_compile_definitions(relightlab
	PUBLIC
		RELIGHT_VERSION=${RELIGHT_VERSION})

if (APPLE)
	set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../build_scripts/relightlab.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../build_scripts/relightlab.icns DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/relightlab.app/Contents/Resources/")
	set_target_properties(relightlab PROPERTIES 
		MACOSX_BUNDLE_ICON_FILE relightlab.icns
		MACOSX_BUNDLE_BUNDLE_VERSION "${RELIGHT_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELIGHT_VERSION}"
		MACOSX_BUNDLE_INFO_STRING "Relight ${RELIGHT_VERSION}"
		MACOSX_BUNDLE_COPYRIGHT "Copyright VCG-ISTI-CNR © 2005-2023. All rights reserved."
		)
	
	set_additional_settings_info_plist(
		TARGET relightlab
		FILE ${CMAKE_CURRENT_BINARY_DIR}/relightlab.app/Contents/Info.plist)
endif()

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	set(RELIGHT_INSTALL_BIN_DIR .)
endif()

install (TARGETS relightlab DESTINATION ${RELIGHT_INSTALL_BIN_DIR})

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION .)
endif()
