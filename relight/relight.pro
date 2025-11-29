QT += core gui widgets concurrent xml

QMAKE_CXXFLAGS += -std=c++17

TARGET = relight
CONFIG -= app_bundle
UI_DIR = $$PWD

TEMPLATE = app

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX
DEFINES += HAVE_LCMS2

INCLUDEPATH += ../external/

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

unix:INCLUDEPATH += /usr/include/eigen3
unix:LIBS += -ljpeg -ltiff -llcms2 -lgomp #-liomp5
unix:QMAKE_CXXFLAGS += -fopenmp


mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/2.0.6/include \
    /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/2.0.6/lib/ -ljpeg -llcms2
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -fopenmp
mac:QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -lomp -I/usr/local/include
mac:QMAKE_LFLAGS += -lomp
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib

win32:LCMS2_HOME = $$getenv(LCMS2_HOME)
win32:!isEmpty(LCMS2_HOME) {
    INCLUDEPATH += $$LCMS2_HOME/include
    LIBS += $$LCMS2_HOME/lib/lcms2.lib
} else {
    LIBS += -llcms2
}

SOURCES += main.cpp \
    ../src/normals/bni_normal_integration.cpp \
    ../src/normals/normals_parameters.cpp \
    dstretchdialog.cpp \
    dstretchtask.cpp \
    history.cpp \
    imagecropper.cpp \
    mainwindow.cpp \
    graphics_view_zoom.cpp \
    normalstask.cpp \
    rtiexport.cpp \
    ../src/cli/convert_rti.cpp \
    ../src/cli/rtibuilder.cpp \
    ../src/imageset.cpp \
    ../src/colorprofile.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/icc_profiles.cpp \
    ../src/rti.cpp \
    ../src/legacy_rti.cpp \
    ../src/deepzoom.cpp \
    ../src/exif.cpp \
    ../src/project.cpp \
    ../src/dome.cpp \
    ../src/normals/flatnormals.cpp \
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
    ../src/lp.cpp \
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
    ../src/colorprofile.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/icc_profiles.h \
    ../src/material.h \
    ../src/eigenpca.h \
    ../src/cli/rtibuilder.h \
    ../src/relight_threadpool.h \
    ../src/project.h \
    ../src/measure.h \
    focaldialog.h \
    ../src/lens.h \
    ../src/image.h \
    ../src/exif.h \
    ../src/normals/flatnormals.h \
    httpserver.h \
    scripts.h \
    processqueue.h \
    queuewindow.h \
    queueitem.h \
    parameter.h \
    task.h \
    rtitask.h \
    settingsdialog.h \
    ../src/legacy_rti.h \
    ../src/network/httplib.h \
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
    zoomtask.h \
    ../src/deepzoom.h

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

