QT += core gui xml
CONFIG += c++17 console
CONFIG -= app_bundle



# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -L /opt/homebrew/lib -ltiff
SOURCES += \
        ../src/bni_normal_integration.cpp \
        camera.cpp \
        depthmap.cpp \
        gaussiangrid.cpp \
        main.cpp \
        orthodepthmap.cpp
#mac:INCLUDEPATH += /opt/homebrew/include
mac:INCLUDEPATH += /opt/homebrew/Cellar/libtiff/4.7.0/include /opt/homebrew/Cellar/eigen/3.4.0_1/include


unix:INCLUDEPATH += ../external/eigen-3.3.9 ../external

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../src/bni_normal_integration.h \
    ../external/assm/Grid.h \
    camera.h \
    depthmap.h \
    gaussiangrid.h \
    orthodepthmap.h
