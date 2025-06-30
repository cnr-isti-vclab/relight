QT += core concurrent

TARGET = relight-cli
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++17

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

INCLUDEPATH += ../external/

win32:INCLUDEPATH += ../libjpeg/include 
win32:LIBS += ../libjpeg/lib/jpeg.lib 

unix:INCLUDEPATH += /usr/include/eigen3
unix:LIBS += -ljpeg -liomp5 -ltiff
unix:QMAKE_CXXFLAGS += -fopenmp

mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/2.0.6/include \
    /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/2.0.6/lib/ -ljpeg 
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -fopenmp
mac:QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -lomp -I/usr/local/include
mac:QMAKE_LFLAGS += -lomp
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib


DESTDIR = "../bin"

SOURCES += main.cpp \
    ../relight/normalstask.cpp \
    ../relight/task.cpp \
    ../src/align.cpp \
    ../src/bni_normal_integration.cpp \
    ../src/dome.cpp \
    ../src/fft_normal_integration.cpp \
    ../src/flatnormals.cpp \
    ../src/getopt.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/lens.cpp \
    ../src/rti.cpp \
    ../src/legacy_rti.cpp \
	../src/sphere.cpp \
    rtibuilder.cpp \
    convert_rti.cpp \
    ../src/lp.cpp

HEADERS += \
    ../relight/normalstask.h \
    ../relight/task.h \
    ../src/bni_normal_integration.h \
    ../src/dome.h \
    ../src/fft_normal_integration.h \
    ../src/flatnormals.h \
    ../src/getopt.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/lens.h \
    ../src/material.h \
    ../src/vector.h \
    ../src/rti.h \
    ../src/legacy_rti.h \
    ../src/eigenpca.h \
	../src/sphere.h \
    rtibuilder.h \
    ../src/lp.h


DISTFILES += \
    plan.txt

