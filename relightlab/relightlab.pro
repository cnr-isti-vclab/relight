QT += widgets xml concurrent
CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += _USE_MATH_DEFINES #WITH_OPENCV not needed now
DEFINES += NOMINMAX
DEFINES += HAVE_LCMS2


INCLUDEPATH += ../external/

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

unix::QMAKE_CXXFLAGS = -fopenmp
unix:INCLUDEPATH += ../external/eigen-3.3.9/ ../src/
#opencv not needed for the moment
#unix:INCLUDEPATH += /usr/include/opencv4
unix:LIBS += -ljpeg -ltiff -lpng -llcms2 -llensfun -lraw -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_calib3d
unix:INCLUDEPATH += /usr/include/lensfun /usr/include/libraw /usr/include/opencv4
#unix:LIBS += -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_video  # kept for reference
unix::LIBS += -fopenmp #-lgomp

mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/3.1.0/include \
    /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/3.1.0/lib/ -ljpeg -llcms2
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -Xpreprocessor -I/usr/local/include #fopenmp -lomp
# mac:QMAKE_LFLAGS += -lomp
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib

win32:LCMS2_HOME = $$getenv(LCMS2_HOME)
win32:!isEmpty(LCMS2_HOME) {
    INCLUDEPATH += $$LCMS2_HOME/include
    LIBS += $$LCMS2_HOME/lib/lcms2.lib
} else {
    LIBS += -llcms2
}


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

SOURCES += main.cpp \
    ../external/assm/SurfaceMesh.cpp \
    ../external/assm/algorithms/DifferentialGeometry.cpp \
    ../external/assm/algorithms/Rasterizer.cpp \
    ../external/assm/algorithms/ScreenRemeshing.cpp \
    ../external/assm/algorithms/Triangulation.cpp \
    ../src/brdf/brdftask.cpp \
    ../src/brdf/brdfparameters.cpp \
    ../src/calibration/calibrationsession.cpp \
    ../src/cli/convert_rti.cpp \
    ../src/cli/rtibuilder.cpp \
    ../src/miniz.c \
    ../src/network/httpserver.cpp \
    ../src/normals/flatnormals.cpp \
    ../src/normals/normalstask.cpp \
    ../src/normals/normalsworker.cpp \
    ../src/normals/bni_normal_integration.cpp \
    ../src/normals/normals_parameters.cpp \
    ../src/normals/fft_normal_integration.cpp \
    ../src/normals/fast_gaussian_blur.cpp \
    ../src/rti/rtiparameters.cpp \
    ../src/rti/rtitask.cpp \
    ../src/crop.cpp \
    ../src/align.cpp \
    ../src/dome.cpp \
    ../src/exif.cpp \
    ../src/lp.cpp \
    ../src/image.cpp \
    ../src/imageset.cpp \
    ../src/colorprofile.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/image_decoder.cpp \
    ../src/tiff_decoder.cpp \
    ../src/png_decoder.cpp \
    ../src/exr_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/icc_profiles.cpp \
    ../src/legacy_rti.cpp \
    ../src/lens.cpp \
    ../src/measure.cpp \
    ../src/project.cpp \
    ../src/rti.cpp \
    ../src/sphere.cpp \
    ../src/white.cpp \
    ../src/task.cpp \
    ../src/deepzoom.cpp \
    calibration/gridcalibview.cpp \
    zoom.cpp \
    brdfplan.cpp \
    imagecropper.cpp \
    processqueue.cpp \
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
    directionsview.cpp \
    verifyview.cpp \
    verifydialog.cpp \
    helpbutton.cpp \
    cropframe.cpp \
    queueitem.cpp \
    queueframe.cpp \
    normalsframe.cpp \
    scaleframe.cpp \
    planrow.cpp \
    normalsplan.cpp \
    planepicking.cpp \
    brdfframe.cpp \
    metadataframe.cpp \
    sphereframe.cpp \
    convertdialog.cpp \
    calibration/calibrationdialog.cpp \
    calibration/caldomeframe.cpp \
    calibration/caldistortionframe.cpp \
    calibration/calflatfieldframe.cpp \
    calibration/callightsframe.cpp \
    ../src/calibration/calibration.cpp \
    historytask.cpp

RESOURCES += \
    ../src/icc_profiles.qrc \
    res.qrc


HEADERS += \
    ../external/assm/algorithms/DifferentialGeometry.h \
    ../external/assm/algorithms/Integration.h \
    ../external/assm/algorithms/PhotometricRemeshing.h \
    ../external/assm/algorithms/Rasterizer.h \
    ../external/assm/algorithms/ScreenDifferentialGeometry.h \
    ../external/assm/algorithms/ScreenMeshing.h \
    ../external/assm/algorithms/ScreenRemeshing.h \
    ../external/assm/algorithms/Triangulation.h \
    ../src/calibration/calibrationsession.h \
    ../src/cli/rtibuilder.h \
    ../src/crop.h \
    ../src/exr_reader.hh \
    ../src/normals/flatnormals.h \
    ../src/streamreader.hh \
    ../src/tinyexr.h \
    brdfplan.h \
    calibration/gridcalibview.h \
    imagecropper.h \
    processqueue.h \
    ../src/align.h \
    ../src/dome.h \
    ../src/exif.h \
    ../src/image.h \
    ../src/imageset.h \
    ../src/colorprofile.h \
    ../src/jpeg_decoder.h \
    ../src/image_decoder.h \
    ../src/tiff_decoder.h \
    ../src/png_decoder.h \
    ../src/exr_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/icc_profiles.h \
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
    verifyview.h \
    verifydialog.h \
    helpbutton.h \
    cropframe.h \
    ../src/rti/rtitask.h \
    ../src/task.h \
    queueitem.h \
    queueframe.h \
    ../src/network/httpserver.h \
    ../src/network/httplib.h \
    normalsframe.h \
    ../src/normals/normalstask.h \
    ../src/normals/normalsworker.h \
    ../src/normals/bni_normal_integration.h \
    ../src/normals/normals_parameters.h \
    scaleframe.h \
    ../src/normals/flatnormals.h \
    planrow.h \
    normalsplan.h \
    planepicking.h \
    brdfframe.h \
    metadataframe.h \
    ../src/normals/fft_normal_integration.h \
    ../src/normals/fast_gaussian_blur.h \
    ../src/normals/pocketfft.h \
    ../src/deepzoom.h \
    sphereframe.h \
    ../src/brdf/brdftask.h \
    convertdialog.h \
    calibration/calibrationdialog.h \
    calibration/caldomeframe.h \
    calibration/caldistortionframe.h \
    calibration/calflatfieldframe.h \
    calibration/callightsframe.h \
    ../src/calibration/calibration.h \
    historytask.h 

FORMS +=

DISTFILES += \
    css/style.qss \
    roadmap.md \
    ../deploy/relight.png


