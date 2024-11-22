#QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        ../src/bni_normal_integration.cpp \
        main.cpp

HEADERS += \
    ../src/bni_normal_integration.h


unix:INCLUDEPATH += /usr/include/eigen3
win32:INCLUDEPATH +=  ../external/eigen-3.4.0/

unix:INCLUDEPATH += /usr/include/eigen3
unix:INCLUDEPATH += ../external/eigen-3.4.0/
unix:LIBS += -lgomp -ltiff #-liomp5
unix:QMAKE_CXXFLAGS += -fopenmp


mac:INCLUDEPATH += /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -fopenmp
mac:QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -lomp -I/usr/local/include
mac:QMAKE_LFLAGS += -lomp
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

