cmake_minimum_required(VERSION 3.13)
project(relight-normals)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_FRAMEWORK LAST)

set(CMAKE_AUTOMOC ON)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
	SET(CMAKE_INSTALL_RPATH $ORIGIN/../Frameworks)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

SET(HEADERS
	../src/normals/bni_normal_integration.h
	../src/normals/fft_normal_integration.h
	../src/normals/flatnormals.h
	../src/normals/pocketfft.h
	../src/normals/normals_parameters.h
	../src/normals/normalstask.h
	../src/task.h
	../src/relight_threadpool.h
	../src/cli/rtibuilder.h
	../src/getopt.h
	../src/imageset.h
	../src/image.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/relight_vector.h
	../src/project.h
	../src/dome.h
	../src/sphere.h
	../src/lens.h
	../src/crop.h
	../src/lp.h
	../src/exif.h
	../src/measure.h
	../src/align.h
	../src/white.h
	../src/rti.h
	../src/legacy_rti.h
	../external/assm/algorithms/Integration.h
	../external/assm/algorithms/DifferentialGeometry.h
	../external/assm/algorithms/Rasterizer.h
	../external/assm/algorithms/Triangulation.h
	../external/assm/algorithms/ScreenRemeshing.h
	../external/assm/algorithms/PhotometricRemeshing.h
	../external/assm/SurfaceMesh.h
	../external/assm/Types.h
)

SET(SOURCES
	main.cpp
	../src/normals/bni_normal_integration.cpp
	../src/normals/fft_normal_integration.cpp
	../src/normals/flatnormals.cpp
	../src/normals/normals_parameters.cpp
	../src/normals/normalstask.cpp
	../src/task.cpp
	../src/threadpool.cpp
	../src/cli/rtibuilder.cpp
	../src/getopt.cpp
	../src/imageset.cpp
	../src/image.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
	../src/project.cpp
	../src/dome.cpp
	../src/sphere.cpp
	../src/lens.cpp
	../src/crop.cpp
	../src/lp.cpp
	../src/exif.cpp
	../src/measure.cpp
	../src/align.cpp
	../src/white.cpp
	../src/rti.cpp
	../src/legacy_rti.cpp
	../external/assm/SurfaceMesh.cpp
	../external/assm/algorithms/DifferentialGeometry.cpp
	../external/assm/algorithms/Rasterizer.cpp
	../external/assm/algorithms/Triangulation.cpp
	../external/assm/algorithms/ScreenRemeshing.cpp
)

add_executable(relight-normals ${SOURCES} ${HEADERS})
target_include_directories(relight-normals PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${JPEG_INCLUDE_DIR}
		../external
)

if(Eigen3_FOUND)
	target_link_libraries(relight-cli PUBLIC Eigen3::Eigen)
else()
	target_include_directories(relight-cli PUBLIC ${EIGEN3_INCLUDE_DIR})
endif()

target_link_libraries(
	relight-normals PUBLIC
		${JPEG_LIBRARIES}
		TIFF::TIFF
		OpenMP::OpenMP_CXX
		${RELIGHT_QT}::Core
		${RELIGHT_QT}::Gui
		${RELIGHT_QT}::Concurrent
)

target_compile_definitions(relight-normals PUBLIC _USE_MATH_DEFINES NOMINMAX)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/relight.app/Contents/MacOS/)
	else()
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX})
	endif()
endif()

install (TARGETS relight-normals 
	DESTINATION ${RELIGHT_INSTALL_BIN_DIR})

if (WIN32)
	install(FILES ${JPEGTURBO_HOME}/bin/jpeg62.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
