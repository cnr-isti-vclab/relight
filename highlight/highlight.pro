QT += core gui widgets concurrent

QMAKE_CXXFLAGS += -std=c++17

TARGET = highlight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

win32:INCLUDEPATH += ../libjpeg/include ../armadillo-9.200.7/include
win32:LIBS += ../libjpeg/lib/jpeg.lib ../armadillo-9.200.7/lib/blas_win64_MT.lib ../armadillo-9.200.7/lib/lapack_win64_MT.lib

unix:LIBS += -larmadillo -ljpeg

SOURCES += main.cpp \
    mainwindow.cpp \
    ball.cpp \
    graphics_view_zoom.cpp \
    rtiexport.cpp \
    ../relight/relight.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/material.cpp \
    ../src/pca.cpp \
    ../src/rti.cpp \
    ../src/utils.cpp \
    helpdialog.cpp


HEADERS += \
    mainwindow.h \
    ball.h \
    graphics_view_zoom.h \
    rtiexport.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/material.h \
    ../src/pca.h \
    helpdialog.h

FORMS += \
    mainwindow.ui \
    rtiexport.ui \
    helpdialog.ui

RESOURCES += \
    icons.qrc

DISTFILES += \
    docs/help.html

