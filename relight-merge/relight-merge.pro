QT += concurrent
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

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

unix:QMAKE_CXXFLAGS = -fopenmp
unix:INCLUDEPATH += ../external/eigen-3.3.9/ /usr/include/eigen3
unix:LIBS += -ljpeg -ltiff
unix:LIBS += -fopenmp

mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/3.1.0/include \
    /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/3.1.0/lib/ -ljpeg
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -Xpreprocessor -I/usr/local/include
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib

SOURCES += main.cpp \
    ../src/getopt.cpp \
    ../src/rti.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/imageset.cpp \
    ../src/lp.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/cli/rtibuilder.cpp \
    ../src/dome.cpp \
    ../src/crop.cpp \
    ../src/sphere.cpp \
    ../src/lens.cpp

HEADERS += \
    ../src/rti.h \
    ../src/jpeg_decoder.h \
    ../src/imageset.h \
    ../src/lp.h \
    ../src/jpeg_encoder.h \
    ../src/cli/rtibuilder.h \
    ../src/dome.h \
    ../src/crop.h \
    ../src/sphere.h \
    ../src/lens.h
