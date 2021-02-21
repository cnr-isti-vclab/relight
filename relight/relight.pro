QT += core gui widgets concurrent

QMAKE_CXXFLAGS += -std=c++17

TARGET = relight
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

win32:INCLUDEPATH += ../libjpeg/include
win32:LIBS += ../libjpeg/lib/jpeg.lib

unix:INCLUDEPATH += /usr/include/eigen3
unix:LIBS += -ljpeg

SOURCES += main.cpp \
    imagecropper.cpp \
    mainwindow.cpp \
    ball.cpp \
    graphics_view_zoom.cpp \
    rtiexport.cpp \
    ../relight-cli/rtibuilder.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/rti.cpp \
    helpdialog.cpp \
    project.cpp \
    measure.cpp


HEADERS += \
    imagecropper.h \
    imagecropper_e.h \
    imagecropper_p.h \
    mainwindow.h \
    ball.h \
    graphics_view_zoom.h \
    rtiexport.h \
    helpdialog.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/material.h \
    ../src/eigenpca.h \
    ../relight-cli/rtibuilder.h \
    project.h \
    measure.h

FORMS += \
    mainwindow.ui \
    rtiexport.ui \
    helpdialog.ui

RESOURCES += \
    icons.qrc

DISTFILES += \
    README.txt \
    docs/help.html

