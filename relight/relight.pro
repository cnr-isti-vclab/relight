QT += core gui widgets concurrent xml

QMAKE_CXXFLAGS += -std=c++17

TARGET = relight
CONFIG -= app_bundle
UI_DIR = $$PWD

TEMPLATE = app

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

unix:INCLUDEPATH += /usr/include/eigen3
unix:LIBS += -ljpeg -ltiff -lgomp #-liomp5
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
    ../src/bni_normal_integration.cpp \
    dstretchdialog.cpp \
    dstretchtask.cpp \
    history.cpp \
    imagecropper.cpp \
    mainwindow.cpp \
    graphics_view_zoom.cpp \
    normalstask.cpp \
    rtiexport.cpp \
    ../relight-cli/convert_rti.cpp \
    ../relight-cli/rtibuilder.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/rti.cpp \
    ../src/legacy_rti.cpp \
    ../src/deepzoom.cpp \
    ../src/exif.cpp \
    ../src/project.cpp \
    ../src/dome.cpp \
    ../src/flatnormals.cpp \
    helpdialog.cpp \
    ../src/measure.cpp \
    focaldialog.cpp \
    ../src/lens.cpp \
    ../src/image.cpp \
    httpserver.cpp \
    scripts.cpp \
    processqueue.cpp \
    queuewindow.cpp \
    queueitem.cpp \
    parameter.cpp \
    task.cpp \
    rtitask.cpp \
    settingsdialog.cpp \
    domecalibration.cpp \
    qmarkerlist.cpp \
    qmarker.cpp \
    qmeasuremarker.cpp \
    ../src/sphere.cpp \
    qspheremarker.cpp \
    ../src/align.cpp \
    qalignmarker.cpp \
    qwhitemarker.cpp \
    ../src/white.cpp \
    convertdialog.cpp \
    aligndialog.cpp \
    zoomdialog.cpp \
    zoomtask.cpp



HEADERS += \
    ../src/bni_normal_integration.h \
    ../src/deepzoom.h \
    dstretch.h \
    dstretchdialog.h \
    dstretchtask.h \
    history.h \
    imagecropper.h \
    mainwindow.h \
    graphics_view_zoom.h \
    normalstask.h \
    rtiexport.h \
    helpdialog.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/material.h \
    ../src/eigenpca.h \
    ../relight-cli/rtibuilder.h \
    ../src/relight_threadpool.h \
    ../src/project.h \
    ../src/measure.h \
    focaldialog.h \
    ../src/lens.h \
    ../src/image.h \
    ../src/exif.h \
    ../src/flatnormals.h \
    httpserver.h \
    scripts.h \
    processqueue.h \
    queuewindow.h \
    queueitem.h \
    parameter.h \
    task.h \
    rtitask.h \
    settingsdialog.h \
    httplib.h \
    domecalibration.h \
    ../src/dome.h \
    ../src/relight_vector.h \
    ../src/lp.h \
    qmarker.h \
    qmarkerlist.h \
    qmeasuremarker.h \
    ../src/sphere.h \
    qspheremarker.h \
    ../src/align.h \
    qalignmarker.h \
    qwhitemarker.h \
    ../src/white.h \
    convertdialog.h \
    aligndialog.h \
    zoom.h \
    zoomdialog.h \
    zoomtask.h

FORMS += \
    dstretchdialog.ui \
    mainwindow.ui \
    rtiexport.ui \
    helpdialog.ui \
    focaldialog.ui \
    zoomdialog.ui   \
    queuewindow.ui \
    settingsdialog.ui \
    domecalibration.ui \
    convertdialog.ui \
    aligndialog.ui
RESOURCES += \
    icons.qrc

DISTFILES += \
    README.txt \
    docs/help.html

