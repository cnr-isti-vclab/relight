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
    ../relight/parameter.h 
    ../relight/processqueue.h 
    ../src/align.h 
    ../src/dome.h 
    ../src/exif.h 
    ../src/image.h 
    ../src/lens.h 
    ../src/measure.h 
    ../src/project.h 
    ../src/sphere.h 
    ../src/white.h 
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
    homeframe.h 
    imagelist.h 
    flowlayout.h 
    imagegrid.h 
    lightsframe.h 
    ../src/lp.h 
    domepanel.h 
    directionsview.h 
    spherepanel.h 
    spherepicking.h 
    spheredialog.h 
    ../relight/task.h 
    verifyview.h 
    verifydialog.h 
    helpbutton.h 
    cropframe.h 
    rtiframe.h
    ../relight/imagecropper.h

)

set (RELIGHTLAB_SOURCES
	main.cpp 
    ../relight/parameter.cpp 
    ../relight/processqueue.cpp 
    ../src/align.cpp 
    ../src/dome.cpp 
    ../src/exif.cpp 
    ../src/image.cpp 
    ../src/lens.cpp 
    ../src/measure.cpp 
    ../src/project.cpp 
    ../src/sphere.cpp 
    ../src/white.cpp 
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
    homeframe.cpp 
    imagelist.cpp 
    flowlayout.cpp 
    imagegrid.cpp 
    lightsframe.cpp 
    ../src/lp.cpp 
    directionsview.cpp 
    spherepanel.cpp 
    spherepicking.cpp 
    spheredialog.cpp 
    ../relight/task.cpp 
    verifyview.cpp 
    verifydialog.cpp 
    helpbutton.cpp 
    cropframe.cpp
    rtiframe.cpp
    ../relight/imagecropper.cpp
)


set (RELIGHTLAB_RESOURCES
	res.qrc
)

add_executable(relightlab ${MACOSX_EXE_TARGET_OPTION} ${RELIGHTLAB_HEADERS} ${RELIGHTLAB_SOURCES} ${RELIGHTLAB_RESOURCES})
target_include_directories(
	relightlab PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		${EIGEN3_INCLUDE_DIR}
	)
	
target_link_libraries(
	relightlab PUBLIC
		${JPEG_LIBRARIES}
		OpenMP::OpenMP_CXX
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
