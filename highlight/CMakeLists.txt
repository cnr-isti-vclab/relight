cmake_minimum_required(VERSION 3.9)
project(relight)

set(CMAKE_CXX_STANDARD 17)

find_package(
	Qt5
	COMPONENTS Core Gui Widgets Concurrent
	REQUIRED)
find_package(Armadillo REQUIRED)
find_package (JPEG REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(${ARMADILLO_INCLUDE_DIRS})

set (RELIGHT_HEADERS
    imagecropper.h
    imagecropper_e.h
    imagecropper_p.h
    mainwindow.h
    ball.h
    graphics_view_zoom.h
    rtiexport.h
    ../src/imageset.h
    ../src/jpeg_decoder.h
    ../src/jpeg_encoder.h
    ../src/material.h
    ../src/pca.h
    helpdialog.h
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
    ../src/pca.cpp
    ../src/rti.cpp
    ../src/utils.cpp
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
target_include_directories(relight PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    relight PUBLIC
        ${ARMADILLO_LIBRARIES}
        ${JPEG_LIBRARIES}
        Qt5::Core
    		Qt5::Gui
    		Qt5::Widgets
    		Qt5::Concurrent)
target_compile_definitions(relight PUBLIC _USE_MATH_DEFINES NOMINMAX)
