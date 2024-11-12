QT += widgets xml concurrent
CONFIG += c++17

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
unix:LIBS += -ljpeg -ltiff -lgomp
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
    ../relight-cli/convert_rti.cpp \
    ../relight-cli/rtibuilder.cpp \
    ../src/flatnormals.cpp \
    processqueue.cpp \
    ../src/align.cpp \
    ../src/dome.cpp \
    ../src/exif.cpp \
    ../src/image.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/legacy_rti.cpp \
    ../src/lens.cpp \
    ../src/measure.cpp \
    ../src/project.cpp \
    ../src/rti.cpp \
    ../src/sphere.cpp \
    ../src/white.cpp \
    alignframe.cpp \
    alignpicking.cpp \
    alignrow.cpp \
    canvas.cpp \
    domepanel.cpp \
    imageview.cpp \
    lightgeometry.cpp \
    mainwindow.cpp \
    markerdialog.cpp \
    preferences.cpp \
    qlabelbutton.cpp \
    recentprojects.cpp \
    reflectionview.cpp \
    relightapp.cpp \
    rticard.cpp \
    rtiframe.cpp \
    rtiplan.cpp \
    rtirecents.cpp \
    rtirow.cpp \
    spherepicking.cpp \
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
    spheredialog.cpp \
    verifyview.cpp \
    verifydialog.cpp \
    helpbutton.cpp \
    cropframe.cpp \
    ../relight/imagecropper.cpp \
    creatertidialog.cpp \
    rtiexportdialog.cpp \
    rtitask.cpp \
    task.cpp \
    queueitem.cpp \
    queueframe.cpp \
    ../relight/httpserver.cpp \
    normalsframe.cpp \
    normalstask.cpp \
    ../src/bni_normal_integration.cpp \
    scaleframe.cpp

RESOURCES += \
    res.qrc


HEADERS += \
    ../relight-cli/rtibuilder.h \
    ../src/flatnormals.h \
    processqueue.h \
    ../src/align.h \
    ../src/dome.h \
    ../src/exif.h \
    ../src/image.h \
    ../src/imageset.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/legacy_rti.h \
    ../src/lens.h \
    ../src/measure.h \
    ../src/project.h \
    ../src/rti.h \
    ../src/sphere.h \
    ../src/white.h \
    alignframe.h \
    alignpicking.h \
    alignrow.h \
    canvas.h \
    imageview.h \
    lightgeometry.h \
    mainwindow.h \
    mainwindow.h \
    markerdialog.h \
    preferences.h \
    qlabelbutton.h \
    recentprojects.h \
    reflectionview.h \
    relightapp.h \
    rticard.h \
    rtiframe.h \
    rtiplan.h \
    rtirecents.h \
    rtirow.h \
    spherepicking.h \
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
    spheredialog.h \
    verifyview.h \
    verifydialog.h \
    helpbutton.h \
    cropframe.h \
    ../relight/imagecropper.h \
    creatertidialog.h \
    rtiexportdialog.h \
    rtitask.h \
    task.h \
    queueitem.h \
    queueframe.h \
    ../relight/httpserver.h \
    ../relight/httplib.h \
    normalsframe.h \
    normalstask.h \
    ../src/bni_normal_integration.h \
    scaleframe.h

FORMS +=

DISTFILES += \
    roadmap.md


