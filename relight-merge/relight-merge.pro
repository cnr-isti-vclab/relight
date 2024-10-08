QT += concurrent
CONFIG += c++11 console
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


win32:INCLUDEPATH += ../libjpeg/include
win32:LIBS += ../libjpeg/lib/jpeg.lib

unix:INCLUDEPATH += /usr/include/eigen3 /usr/include/python3.6m
#unix:LIBS += -ljpeg -liomp5
#unix:QMAKE_CXXFLAGS += -fopenmp

mac:INCLUDEPATH += /opt/homebrew/opt/jpeg-turbo/include \
    /usr/local/include \
    /usr/local/include/eigen3 \
    /opt/homebrew/opt/jpeg/include \
    /opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3
   # /usr/local/include/eigen3
mac:LIBS += -L/opt/homebrew/opt/jpeg-turbo/lib/ -ljpeg
mac:LIBS += -framework Accelerate
#mac:QMAKE_CXXFLAGS += -fopenmp
#mac:QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -lomp -I/usr/local/include
#mac:QMAKE_LFLAGS += -lomp
#mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib
mac:QMAKE_CXXFLAGS += -Xpreprocessor
#mac:QMAKE_LFLAGS += -lomp
mac:LIBS +=  -L/opt/homebrew/opt/jpeg/lib

SOURCES += main.cpp \
    ../src/getopt.cpp \
    ../src/rti.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/imageset.cpp \
    ../src/lp.cpp \
    ../src/jpeg_encoder.cpp \
    ../relight-cli/rtibuilder.cpp

HEADERS += \
    ../src/rti.h \
    ../src/jpeg_decoder.h \
    ../src/imageset.h \
    ../src/lp.h \
    ../src/jpeg_encoder.h \
    ../relight-cli/rtibuilder.h
