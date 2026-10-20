cmake_minimum_required(VERSION 3.13)
project(relight-pano)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

set(DEPTHMAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../relight-depthmap)
set(HEADERS
	exiftransplant.h
	mainwindow.h
	orixml.h
	panobuilder.h
	${DEPTHMAP_DIR}/camera.h
	${DEPTHMAP_DIR}/depthmap.h
	${DEPTHMAP_DIR}/gaussiangrid.h
	${DEPTHMAP_DIR}/orthodepthmap.h
	../src/normals/bni_normal_integration.h
)

set(SOURCES
	main.cpp
	exiftransplant.cpp
	mainwindow.cpp
	orixml.cpp
	panobuilder.cpp
	${DEPTHMAP_DIR}/camera.cpp
	${DEPTHMAP_DIR}/depthmap.cpp
	${DEPTHMAP_DIR}/gaussiangrid.cpp
	${DEPTHMAP_DIR}/orthodepthmap.cpp
	../src/normals/bni_normal_integration.cpp
)

add_executable(relight-pano ${HEADERS} ${SOURCES})
target_include_directories(relight-pano PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${DEPTHMAP_DIR}
	../external
	../src
	../src/normals
)
if(Eigen3_FOUND)
	target_link_libraries(relight-pano PRIVATE Eigen3::Eigen)
else()
	target_include_directories(relight-pano PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()
target_link_libraries(relight-pano PRIVATE
	TIFF::TIFF
	Threads::Threads
	${RELIGHT_QT}::Core
	${RELIGHT_QT}::Gui
	${RELIGHT_QT}::Widgets
	${RELIGHT_QT}::Xml
)

if (INSTALL_TO_UNIX_LAYOUT)
	set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR})
else()
	if (APPLE AND MACOSX_MAKE_BUNDLE)
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/relightlab.app/Contents/MacOS/)
	else()
		set(RELIGHT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX})
	endif()
endif()

install(TARGETS relight-pano DESTINATION ${RELIGHT_INSTALL_BIN_DIR})
