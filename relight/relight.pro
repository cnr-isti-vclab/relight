QT += core

TARGET = relight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

win32:INCLUDEPATH += ../libjpeg/include ../armadillo-9.200.7/include
win32:LIBS += ../libjpeg/lib/jpeg.lib ../armadillo-9.200.7/lib/blas_win64_MT.lib ../armadillo-9.200.7/lib/lapack_win64_MT.lib

unix:LIBS += -larmadillo -ljpeg 

DESTDIR = "../bin"

SOURCES += main.cpp \
    ../src/getopt.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/material.cpp \
    relight.cpp \
    ../src/rti.cpp \
    ../src/pca.cpp \
    ../src/utils.cpp \
    ../src/legacy_rti.cpp

HEADERS += \
    ../src/getopt.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/material.h \
    ../src/vector.h \
    ../src/rti.h \
    ../src/legacy_rti.h \
    rtibuilder.h

DISTFILES += \
    plan.txt

