QT += core concurrent

TARGET = relight-cli
CONFIG += console c++17
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

INCLUDEPATH += ../external/

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

unix:QMAKE_CXXFLAGS = -fopenmp
unix:INCLUDEPATH += ../external/eigen-3.3.9/ /usr/include/eigen3
unix:LIBS += -ljpeg -ltiff
unix:LIBS += -fopenmp

mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/3.1.0/include \
    /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/3.1.0/lib/ -ljpeg
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -Xpreprocessor -I/usr/local/include
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib


DESTDIR = "../bin"

SOURCES += main.cpp \
    ../external/assm/SurfaceMesh.cpp \
	../external/assm/algorithms/DifferentialGeometry.cpp \
	../external/assm/algorithms/Rasterizer.cpp \
	../external/assm/algorithms/ScreenRemeshing.cpp \
	../external/assm/algorithms/Triangulation.cpp \
    ../src/brdf/brdftask.cpp \
    ../src/dome.cpp \
    ../src/getopt.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/lens.cpp \
    ../src/normals/bni_normal_integration.cpp \
    ../src/normals/fft_normal_integration.cpp \
    ../src/normals/flatnormals.cpp \
    ../src/normals/normals_parameters.cpp \
    ../src/normals/normalstask.cpp \
    ../src/rti.cpp \
    ../src/legacy_rti.cpp \
	../src/sphere.cpp \
    ../src/cli/rtibuilder.cpp \
    ../src/cli/convert_rti.cpp \
    ../src/lp.cpp \
    ../src/crop.cpp \
    ../src/task.cpp

HEADERS += \
    ../external/assm/algorithms/DifferentialGeometry.h \
	../external/assm/algorithms/Integration.h \
	../external/assm/algorithms/PhotometricRemeshing.h \
	../external/assm/algorithms/Rasterizer.h \
	../external/assm/algorithms/ScreenDifferentialGeometry.h \
	../external/assm/algorithms/ScreenMeshing.h \
	../external/assm/algorithms/ScreenRemeshing.h \
	../external/assm/algorithms/Triangulation.h \
	../src/brdf/brdftask.h \
    ../src/dome.h \
    ../src/getopt.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/lens.h \
    ../src/material.h \
    ../src/normals/bni_normal_integration.h \
    ../src/normals/fft_normal_integration.h \
    ../src/normals/flatnormals.h \
    ../src/normals/normals_parameters.h \
    ../src/normals/normalstask.h \
    ../src/normals/pocketfft.h \
    ../src/task.h \
    ../src/vector.h \
    ../src/rti.h \
    ../src/legacy_rti.h \
    ../src/eigenpca.h \
	../src/sphere.h \
    ../src/cli/rtibuilder.h \
    ../src/lp.h \
    ../src/crop.h


DISTFILES += \
    plan.txt

