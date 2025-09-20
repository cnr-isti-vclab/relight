#QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        ../src/normals/bni_normal_integration.cpp \
        ../src/normals/fft_normal_integration.cpp \
        ../src/normals/flatnormals.cpp \
        ../src/normals/normals_parameters.cpp \
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
        ../src/rti.cpp \
        ../src/legacy_rti.cpp \
        ../external/assm/algorithms/Integration.cpp \
        ../external/assm/SurfaceMesh.cpp \
        main.cpp

HEADERS += \
    ../src/normals/bni_normal_integration.h \
    ../src/normals/fft_normal_integration.h \
    ../src/normals/flatnormals.h \
    ../src/normals/pocketfft.h \
    ../src/normals/normals_parameters.h \
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
    ../src/rti.h \
    ../src/legacy_rti.h \
    ../external/assm/algorithms/Integration.h \
    ../external/assm/SurfaceMesh.h \
    ../external/assm/Types.h


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

