QT += core gui xml
CONFIG += c++11 console
CONFIG -= app_bundle



# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -L /opt/homebrew/lib -ltiff
SOURCES += \
        ../src/bni_normal_integration.cpp \
        depthmap.cpp \
        main.cpp
INCLUDEPATH += /opt/homebrew/include \
    /opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../src/bni_normal_integration.h \
    depthmap.h
