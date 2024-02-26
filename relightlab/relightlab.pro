QT += widgets
CONFIG += c++11

#TODO: this might be needed in CMake
#find_package(Qt5Svg REQUIRED)
#target_link_libraries( ${APP_NAME} Qt5::Svg )

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

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


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

SOURCES += main.cpp \
    ../relight/parameter.cpp \
    ../relight/processqueue.cpp \
    ../src/align.cpp \
    ../src/dome.cpp \
    ../src/exif.cpp \
    ../src/image.cpp \
    ../src/lens.cpp \
    ../src/measure.cpp \
    ../src/project.cpp \
    ../src/sphere.cpp \
    ../src/white.cpp \
    canvas.cpp \
    domepanel.cpp \
    lightgeometry.cpp \
    mainwindow.cpp \
    preferences.cpp \
    recentprojects.cpp \
    reflectionview.cpp \
    relightapp.cpp \
    rtiframe.cpp \
    sphererow.cpp \
    tabwidget.cpp \
    imageframe.cpp \
    homeframe.cpp \
    imagelist.cpp \
    flowlayout.cpp \
    imagegrid.cpp \
    lightsframe.cpp \
    ../src/lp.cpp \
    directionsview.cpp \
    spherepanel.cpp \
    spherepicking.cpp \
    spheredialog.cpp \
    ../relight/task.cpp \
    verifyview.cpp \
    verifydialog.cpp \
    helpbutton.cpp \
    cropframe.cpp \
    ../relight/imagecropper.cpp

RESOURCES += \
    res.qrc


HEADERS += \
    ../relight/parameter.h \
    ../relight/processqueue.h \
    ../src/align.h \
    ../src/dome.h \
    ../src/exif.h \
    ../src/image.h \
    ../src/lens.h \
    ../src/measure.h \
    ../src/project.h \
    ../src/sphere.h \
    ../src/white.h \
    canvas.h \
    lightgeometry.h \
    mainwindow.h \
    mainwindow.h \
    preferences.h \
    recentprojects.h \
    reflectionview.h \
    relightapp.h \
    rtiframe.h \
    sphererow.h \
    tabwidget.h \
    imageframe.h \
    homeframe.h \
    imagelist.h \
    flowlayout.h \
    imagegrid.h \
    lightsframe.h \
    ../src/lp.h \
    domepanel.h \
    directionsview.h \
    spherepanel.h \
    spherepicking.h \
    spheredialog.h \
    ../relight/task.h \
    verifyview.h \
    verifydialog.h \
    helpbutton.h \
    cropframe.h \
    ../relight/imagecropper.h

FORMS += \
    form.ui

DISTFILES += \
    roadmap.md


