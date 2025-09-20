QT += core concurrent

TARGET = relight-cli
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++17

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

win32:INCLUDEPATH += ../libjpeg/include 
win32:LIBS += ../libjpeg/lib/jpeg.lib 

unix:INCLUDEPATH += /usr/include/eigen3
unix:LIBS += -ljpeg -liomp5
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
    ../src/dome.cpp \
    ../src/getopt.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/lens.cpp \
    ../src/rti.cpp \
    ../src/legacy_rti.cpp \
	../src/sphere.cpp \
    ../src/cli/rtibuilder.cpp \
    ../src/cli/convert_rti.cpp \
    ../src/lp.cpp \
    ../src/crop.cpp

HEADERS += \
    ../src/dome.h \
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
    ../src/cli/rtibuilder.h \
    ../src/lp.h \
    ../src/crop.h


DISTFILES += \
    plan.txt

