cmake_minimum_required(VERSION 3.13)
project(relight-cli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()


SET(HEADERS
	../src/cli/rtibuilder.h
	../src/getopt.h
	../src/imageset.h
	../src/exif.h
	../src/colorprofile.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/icc_profiles.h
	../src/material.h
	../src/relight_vector.h
	../src/rti.h
	../src/legacy_rti.h
	../src/dome.h
	../src/sphere.h
	../src/lens.h
	../src/crop.h
	../src/brdf/brdftask.h 
	../external/assm/algorithms/DifferentialGeometry.h
	../external/assm/algorithms/Integration.h
	../external/assm/algorithms/PhotometricRemeshing.h
	../external/assm/algorithms/Rasterizer.h
	../external/assm/algorithms/ScreenDifferentialGeometry.h
	../external/assm/algorithms/ScreenMeshing.h
	../external/assm/algorithms/ScreenRemeshing.h
	../external/assm/algorithms/Triangulation.h
	../src/normals/bni_normal_integration.h
	../src/normals/fft_normal_integration.h
	../src/normals/pocketfft.h
	../src/normals/flatnormals.h
	../src/normals/normals_parameters.h
	../src/normals/normalstask.h
	../src/task.h
)

SET(SOURCES
	main.cpp
	../src/cli/rtibuilder.cpp
	../src/getopt.cpp
	../src/imageset.cpp
	../src/exif.cpp
	../src/colorprofile.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
	../src/icc_profiles.cpp
	../src/icc_profiles.qrc
	../src/rti.cpp
	../src/legacy_rti.cpp
	../src/lp.cpp
	../src/dome.cpp
	../src/lens.cpp
	../src/sphere.cpp
	../src/crop.cpp
	../src/cli/convert_rti.cpp
	../src/brdf/brdftask.cpp
	../external/assm/SurfaceMesh.cpp
	../external/assm/algorithms/DifferentialGeometry.cpp
	../external/assm/algorithms/Rasterizer.cpp
	../external/assm/algorithms/ScreenRemeshing.cpp
	../external/assm/algorithms/Triangulation.cpp
	../src/normals/bni_normal_integration.cpp
	../src/normals/fft_normal_integration.cpp
	../src/normals/flatnormals.cpp
	../src/normals/normals_parameters.cpp
	../src/normals/normalstask.cpp
	../src/task.cpp
)



add_executable(relight-cli ${HEADERS} ${SOURCES})
target_include_directories(
	relight-cli PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		TIFF::TIFF
		../external
)

if(Eigen3_FOUND)
	target_link_libraries(relight-cli PUBLIC Eigen3::Eigen)
else()
	target_include_directories(relight-cli PUBLIC ${EIGEN3_INCLUDE_DIR})
endif()


target_link_libraries(
	relight-cli PUBLIC
		${JPEG_LIBRARIES}
		TIFF::TIFF
		OpenMP::OpenMP_CXX
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
		${RELIGHT_QT}::Concurrent
		${LCMS2_LIBRARIES})

target_include_directories(relight-cli PRIVATE ${LCMS2_INCLUDE_DIRS})
target_compile_definitions(relight-cli PUBLIC HAVE_LCMS2)

target_compile_definitions(relight-cli PUBLIC _USE_MATH_DEFINES NOMINMAX)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/relightlab.app/Contents/MacOS/)
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
