#export CMAKE_PREFIX_PATH=path to Qt/5.5/gcc_64/ (save for bin/qmake)

cmake_minimum_required(VERSION 3.1)
project(relight-cli)

set (CMAKE_CXX_STANDARD 11)

SET(HEADERS
	rtibuilder.h
	../src/getopt.h
	../src/imageset.h
	../src/jpeg_decoder.h
	../src/jpeg_encoder.h
	../src/material.h
	../src/vector.h
	../src/rti.h
	../src/legacy_rti.h
)

SET(SOURCES
	main.cpp
	rtibuilder.cpp
	../src/getopt.cpp
	../src/imageset.cpp
	../src/jpeg_decoder.cpp
	../src/jpeg_encoder.cpp
	../src/material.cpp
	../src/rti.cpp
	../src/pca.cpp
	../src/utils.cpp
	../src/legacy_rti.cpp
)

cmake_minimum_required(VERSION 3.1)

project(relight)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets)

add_executable(relight-cli ${SOURCES} ${HEADERS})

target_link_libraries(relight-cli Qt5::Widgets jpeg)

INSTALL(TARGETS relight-cli
	RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)





