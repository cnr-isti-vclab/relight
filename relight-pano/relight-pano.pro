QT       += core gui
QT       += xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
LIBS += -L /opt/homebrew/lib -ltiff

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    exiftransplant.cpp \
    main.cpp \
    mainwindow.cpp \
    orixml.cpp \
    panobuilder.cpp \
    ../depthmap/camera.cpp \
    ../depthmap/depthmap.cpp \
    ../depthmap/gaussiangrid.cpp \
    ../depthmap/orthodepthmap.cpp \
    ../src/bni_normal_integration.cpp

HEADERS += \
    exiftransplant.h \
    mainwindow.h \
    orixml.h \
    panobuilder.h \
    ../depthmap/camera.h \
    ../depthmap/depthmap.h \
    ../depthmap/gaussiangrid.h \
    ../depthmap/orthodepthmap.h \
    ../src/bni_normal_integration.h


mac:INCLUDEPATH += /opt/homebrew/include/eigen3 /opt/homebrew/Cellar/libtiff/4.7.0/include /opt/homebrew/Cellar/eigen/3.4.0_1/include ../depthmap
unix:INCLUDEPATH += ../external/eigen-3.3.9



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
