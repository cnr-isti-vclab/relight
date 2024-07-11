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

#unix:LIBS += -ljpeg -liomp5
#unix:QMAKE_CXXFLAGS += -fopenmp

mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/2.0.6/include \
    /usr/local/include \
    /usr/local/include/eigen3 \
    /opt/homebrew/opt/jpeg/include \
    /opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3

mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/2.0.6/lib/ -ljpeg 
mac:LIBS += -framework Accelerate
#mac:QMAKE_CXXFLAGS += -fopenmp
mac:QMAKE_CXXFLAGS += -Xpreprocessor
#mac:QMAKE_LFLAGS += -lomp
mac:LIBS +=  -L/opt/homebrew/opt/jpeg/lib


DESTDIR = "../bin"

SOURCES += main.cpp \
    ../src/getopt.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/rti.cpp \
    ../src/legacy_rti.cpp \
    rtibuilder.cpp \
    convert_rti.cpp \
    ../src/lp.cpp

HEADERS += \
    ../src/getopt.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/material.h \
    ../src/relight_vector.h \
    ../src/rti.h \
    ../src/legacy_rti.h \
    ../src/eigenpca.h \
    rtibuilder.h \
    ../src/lp.h


DISTFILES += \
    plan.txt

