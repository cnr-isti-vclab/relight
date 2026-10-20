cmake_minimum_required(VERSION 3.13)
project(relight-seam)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc stitching)

set(HEADERS
	ortholoader.h
	../src/image_decoder.h
	../src/jpeg_decoder.h
	../src/tiff_decoder.h
	../src/png_decoder.h
	../src/exr_decoder.h
	../src/image_format.h
	../src/miniz.h
)

set(SOURCES
	main.cpp
	ortholoader.cpp
	../src/image_decoder.cpp
	../src/jpeg_decoder.cpp
	../src/tiff_decoder.cpp
	../src/png_decoder.cpp
	../src/exr_decoder.cpp
	../src/miniz.c
)

add_executable(relight-seam ${HEADERS} ${SOURCES})
target_include_directories(relight-seam PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${OpenCV_INCLUDE_DIRS}
	${JPEG_INCLUDE_DIR}
	../src
)
target_link_libraries(relight-seam PRIVATE
	${OpenCV_LIBS}
	${JPEG_LIBRARIES}
	TIFF::TIFF
	PNG::PNG
	${RELIGHT_QT}::Core
	${RELIGHT_QT}::Gui
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

install(TARGETS relight-seam DESTINATION ${RELIGHT_INSTALL_BIN_DIR})
