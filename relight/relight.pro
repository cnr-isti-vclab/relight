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
    measure.cpp \
    focaldialog.cpp \
    lens.cpp \
    image.cpp \
    ../src/exif.cpp \
    httpserver.cpp \
    scripts.cpp \
    processqueue.cpp \
    queuewindow.cpp \
    queueitem.cpp \
    script.cpp \
    parameter.cpp \
    task.cpp \
    rtitask.cpp \
    settingsdialog.cpp \
    ../relight-cli/convert_rti.cpp \
    ../src/legacy_rti.cpp \
    domecalibration.cpp \
    dome.cpp \
    ../src/lp.cpp


HEADERS += \
    imagecropper.h \
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
    measure.h \
    focaldialog.h \
    lens.h \
    image.h \
    ../src/exif.h \
    httpserver.h \
    scripts.h \ 
    processqueue.h \
    queuewindow.h \
    queueitem.h \
    script.h \
    parameter.h \
    task.h \
    rtitask.h \
    settingsdialog.h \
    ../src/legacy_rti.h \
    httplib.h \
    domecalibration.h \
    dome.h \
    ../src/vector.h \
    ../src/lp.h

FORMS += \
    mainwindow.ui \
    rtiexport.ui \
    helpdialog.ui \
    focaldialog.ui \  
    queuewindow.ui \
    settingsdialog.ui \
    domecalibration.ui
RESOURCES += \
    icons.qrc

DISTFILES += \
    README.txt \
    docs/help.html

