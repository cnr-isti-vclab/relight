QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:LIBS += -ljpeg -ltiff -lpng -lstdc++fs
INCLUDEPATH += ../src/

SOURCES += main.cpp \
    ../src/deepzoom.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/image_decoder.cpp \
    ../src/tiff_decoder.cpp \
    ../src/png_decoder.cpp \
    ../src/exr_decoder.cpp \
    ../src/miniz.c

HEADERS += \
    ../src/deepzoom.h \
    ../src/jpeg_encoder.h \
    ../src/jpeg_decoder.h \
    ../src/image_decoder.h \
    ../src/tiff_decoder.h \
    ../src/png_decoder.h \
    ../src/exr_decoder.h \
    ../src/miniz.h
