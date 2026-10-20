cmake_minimum_required(VERSION 3.13)
project(relight-depthmap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

set(HEADERS
	camera.h
	depthmap.h
	gaussiangrid.h
	orthodepthmap.h
	../src/normals/bni_normal_integration.h
)

set(SOURCES
	main.cpp
	camera.cpp
	depthmap.cpp
	gaussiangrid.cpp
	orthodepthmap.cpp
	../src/normals/bni_normal_integration.cpp
)

add_executable(relight-depthmap ${HEADERS} ${SOURCES})
target_include_directories(relight-depthmap PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	../external
	../src
	../src/normals
)
if(Eigen3_FOUND)
	target_link_libraries(relight-depthmap PRIVATE Eigen3::Eigen)
else()
	target_include_directories(relight-depthmap PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()
target_link_libraries(relight-depthmap PRIVATE
	TIFF::TIFF
	Threads::Threads
	${RELIGHT_QT}::Core
	${RELIGHT_QT}::Gui
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

install(TARGETS relight-depthmap DESTINATION ${RELIGHT_INSTALL_BIN_DIR})
