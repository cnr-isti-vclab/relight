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
	../src/rti.h
	../src/legacy_rti.h
	../external/assm/algorithms/Integration.h
	../external/assm/SurfaceMesh.h
	../external/assm/Types.h
)

SET(SOURCES
	main.cpp
	../src/normals/bni_normal_integration.cpp
	../src/normals/fft_normal_integration.cpp
	../src/normals/flatnormals.cpp
	../src/normals/normals_parameters.cpp
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
	../src/rti.cpp
	../src/legacy_rti.cpp
	../external/assm/algorithms/Integration.cpp
	../external/assm/SurfaceMesh.cpp
)

add_executable(relight-normals ${SOURCES} ${HEADERS})

find_package(${RELIGHT_QT} REQUIRED COMPONENTS Core Gui)
target_link_libraries(relight-normals PRIVATE ${RELIGHT_QT}::Core ${RELIGHT_QT}::Gui)

if(Eigen3_FOUND)
	message(STATUS "Found Eigen3")
	target_link_libraries(relight-normals PRIVATE Eigen3::Eigen)
else()
	message(STATUS "Eigen3 not found, using bundled version")
	if(UNIX AND NOT APPLE)
		target_include_directories(relight-normals PRIVATE /usr/include/eigen3)
	endif()
	target_include_directories(relight-normals PRIVATE ../external/eigen-3.4.0)
endif()

if(OpenMP_FOUND)
	target_link_libraries(relight-normals PRIVATE OpenMP::OpenMP_CXX)
endif()

if(WIN32)
	target_link_libraries(relight-normals PRIVATE ws2_32)
endif()

if (APPLE)
	target_link_libraries(relight-normals PRIVATE "-framework Accelerate")
endif()

if(UNIX AND NOT APPLE)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(TIFF REQUIRED libtiff-4)
	target_link_libraries(relight-normals PRIVATE ${TIFF_LIBRARIES})
	target_include_directories(relight-normals PRIVATE ${TIFF_INCLUDE_DIRS})
	target_compile_options(relight-normals PRIVATE ${TIFF_CFLAGS_OTHER})
endif()

if(APPLE OR WIN32)
	target_link_libraries(relight-normals PRIVATE tiff)
	target_include_directories(relight-normals PRIVATE ../external/tiff-4.7.0/libtiff)
endif()

# Installation
if(INSTALL_TO_UNIX_LAYOUT)
	install(TARGETS relight-normals RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
	install(TARGETS relight-normals RUNTIME DESTINATION .)
endif()