cmake_minimum_required(VERSION 3.16)
project(brdflab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_FIND_FRAMEWORK LAST)

find_package(
    ${RELIGHT_QT}
    COMPONENTS Core Gui Widgets Concurrent Xml
    REQUIRED)
find_package(OpenMP)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
    set(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

set(BRDFLAB_HEADERS
    mainwindow.h
    ../src/brdf/brdf_ceres.h
    ../src/brdf/brdf_math.h
    ../src/brdf/brdf_optimizer.h
    ../src/brdf/brdfparameters.h
    ../src/brdf/brdftask.h
    ../src/brdf/init_normals.h
    ../relightlab/canvas.h
    ../relightlab/imageview.h
)

set(BRDFLAB_SOURCES
    main.cpp
    mainwindow.cpp
    ../src/align.cpp
    ../src/brdf/brdf_math.cpp
    ../src/brdf/brdf_optimizer.cpp
    ../src/brdf/brdfparameters.cpp
    ../src/brdf/brdftask.cpp
    ../src/brdf/init_normals.cpp
    ../src/colorprofile.cpp
    ../src/crop.cpp
    ../src/deepzoom.cpp
    ../src/dome.cpp
    ../src/exif.cpp
    ../src/icc_profiles.cpp
    ../src/image.cpp
    ../src/imageset.cpp
    ../src/jpeg_decoder.cpp
    ../src/jpeg_encoder.cpp
    ../src/legacy_rti.cpp
    ../src/lens.cpp
    ../src/lp.cpp
    ../src/measure.cpp
    ../src/network/httpserver.cpp
    ../src/normals/bni_normal_integration.cpp
    ../src/normals/fast_gaussian_blur.cpp
    ../src/normals/fft_normal_integration.cpp
    ../src/normals/flatnormals.cpp
    ../src/normals/normals_parameters.cpp
    ../src/normals/normalstask.cpp
    ../src/project.cpp
    ../src/rti.cpp
    ../src/rti/rtiparameters.cpp
    ../src/rti/rtitask.cpp
    ../src/sphere.cpp
    ../src/task.cpp
    ../src/white.cpp
    ../relightlab/canvas.cpp
    ../relightlab/imageview.cpp
)

set(BRDFLAB_RESOURCES
    ../src/icc_profiles.qrc
)

add_executable(brdflab WIN32 ${MACOSX_EXE_TARGET_OPTION}
    ${BRDFLAB_HEADERS}
    ${BRDFLAB_SOURCES}
    ${BRDFLAB_RESOURCES}
)

target_include_directories(brdflab PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JPEG_INCLUDE_DIR}
    ../external
    ../src
    ../relightlab
)

if(Eigen3_FOUND)
    target_link_libraries(brdflab PUBLIC Eigen3::Eigen)
else()
    target_include_directories(brdflab PUBLIC ${EIGEN3_INCLUDE_DIR})
endif()

find_package(Ceres REQUIRED)

target_link_libraries(brdflab PUBLIC
    Ceres::ceres
    ${JPEG_LIBRARIES}
    TIFF::TIFF
    ${RELIGHT_QT}::Core
    ${RELIGHT_QT}::Gui
    ${RELIGHT_QT}::Widgets
    ${RELIGHT_QT}::Concurrent
    ${RELIGHT_QT}::Xml
    ${LCMS2_LIBRARIES}
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(brdflab PUBLIC OpenMP::OpenMP_CXX)
endif()

target_include_directories(brdflab PRIVATE ${LCMS2_INCLUDE_DIRS})
target_compile_definitions(brdflab PUBLIC HAVE_LCMS2 _USE_MATH_DEFINES NOMINMAX)

if(INSTALL_TO_UNIX_LAYOUT)
    install(TARGETS brdflab DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(TARGETS brdflab DESTINATION .)
endif()
