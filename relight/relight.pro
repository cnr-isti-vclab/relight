QT += core
#QT -= gui

TARGET = relight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11


INCLUDEPATH += ../libpca-1.3.3/src/lib

LIBS += -larmadillo -ljpeg 

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
    relight.h \
    ../src/rti.h \
    ../src/legacy_rti.h \
    rtibuilder.h

DISTFILES += \
    plan.txt

