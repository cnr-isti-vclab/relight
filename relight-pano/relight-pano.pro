QT       += core gui
QT       += xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
PKGCONFIG += opencv4
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
    ../relight-depthmap/camera.cpp \
    ../relight-depthmap/depthmap.cpp \
    ../relight-depthmap/gaussiangrid.cpp \
    ../relight-depthmap/orthodepthmap.cpp \
    ../src/normals/bni_normal_integration.cpp

HEADERS += \
    exiftransplant.h \
    mainwindow.h \
    orixml.h \
    panobuilder.h \
    ../relight-depthmap/camera.h \
    ../relight-depthmap/depthmap.h \
    ../relight-depthmap/gaussiangrid.h \
    ../relight-depthmap/orthodepthmap.h \
    ../src/normals/bni_normal_integration.h

mac:INCLUDEPATH += /opt/homebrew/include/eigen3 /opt/homebrew/Cellar/libtiff/4.7.0/include /opt/homebrew/Cellar/eigen/3.4.0_1/include ../relight-depthmap
unix:INCLUDEPATH += ../external/eigen-3.3.9 ../relight-depthmap



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
