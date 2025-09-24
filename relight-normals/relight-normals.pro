#QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        ../src/normals/bni_normal_integration.cpp \
        ../src/normals/fft_normal_integration.cpp \
        ../src/normals/flatnormals.cpp \
        ../src/normals/normals_parameters.cpp \
        ../src/normals/normalstask.cpp \
        ../src/cli/rtibuilder.cpp \
        ../src/getopt.cpp \
        ../src/imageset.cpp \
        ../src/image.cpp \
        ../src/jpeg_decoder.cpp \
        ../src/jpeg_encoder.cpp \
        ../src/project.cpp \
        ../src/dome.cpp \
        ../src/sphere.cpp \
        ../src/lens.cpp \
        ../src/crop.cpp \
        ../src/lp.cpp \
        ../src/exif.cpp \
        ../src/measure.cpp \
        ../src/align.cpp \
        ../src/white.cpp \
        ../src/rti.cpp \
        ../src/legacy_rti.cpp \
        ../src/task.cpp \
        ../src/threadpool.cpp \
        ../external/assm/SurfaceMesh.cpp \
        ../external/assm/algorithms/DifferentialGeometry.cpp \
        ../external/assm/algorithms/Rasterizer.cpp \
        ../external/assm/algorithms/ScreenRemeshing.cpp \
        ../external/assm/algorithms/Triangulation.cpp \
        main.cpp

HEADERS += \
    ../src/normals/bni_normal_integration.h \
    ../src/normals/fft_normal_integration.h \
    ../src/normals/flatnormals.h \
    ../src/normals/normalstask.h \
    ../src/normals/pocketfft.h \
    ../src/normals/normals_parameters.h \
    ../src/task.h \
    ../src/relight_threadpool.h \
    ../src/cli/rtibuilder.h \
    ../src/getopt.h \
    ../src/imageset.h \
    ../src/image.h \
    ../src/jpeg_decoder.h \
    ../src/jpeg_encoder.h \
    ../src/relight_vector.h \
    ../src/project.h \
    ../src/dome.h \
    ../src/sphere.h \
    ../src/lens.h \
    ../src/crop.h \
    ../src/lp.h \
    ../src/exif.h \
    ../src/measure.h \
    ../src/align.h \
    ../src/white.h \
    ../src/rti.h \
    ../src/legacy_rti.h \
    ../external/assm/algorithms/Integration.h \
    ../external/assm/algorithms/DifferentialGeometry.h \
    ../external/assm/algorithms/Rasterizer.h \
    ../external/assm/algorithms/Triangulation.h \
    ../external/assm/algorithms/ScreenRemeshing.h \
    ../external/assm/algorithms/PhotometricRemeshing.h \
    ../external/assm/SurfaceMesh.h \
    ../external/assm/Types.h

INCLUDEPATH += ../external/

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

