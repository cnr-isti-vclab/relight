QT += core
#QT -= gui

TARGET = rti-quality
CONFIG += console
CONFIG -= app_bundle

QMAKE_CXXFLAGS += -std=c++11

DEFINES += DEBUG_RTI

TEMPLATE = app

LIBS += -ljpeg -ltiff

DESTDIR = "../bin"


SOURCES += main.cpp \
    ../src/getopt.cpp \
    ../src/imageset.cpp \
    ../src/colorprofile.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/image_decoder.cpp \
    ../src/tiff_decoder.cpp \
    ../src/jpeg_encoder.cpp \
	../src/rti.cpp

HEADERS += \
    ../src/getopt.h \
    ../src/imageset.h \
    ../src/vector.h \
    ../src/jpeg_decoder.h \
    ../src/image_decoder.h \
    ../src/tiff_decoder.h \
    ../src/jpeg_encoder.h \
	../src/rti.h
