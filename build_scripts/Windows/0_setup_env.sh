#!/bin/bash
# this is a script shell sets up a Windows environment where
# RelightLab can be compiled, deployed and packaged.
#
# Run this script if you never installed any of the RelightLab dependencies.
#
# Requires: choco

DONT_INSTALL_QT=false

#checking for parameters
for i in "$@"
do
case $i in
    --dont_install_qt)
        DONT_INSTALL_QT=true
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

choco install cmake ninja ccache wget nsis

if [ "$DONT_INSTALL_QT" = false ] ; then
    echo "=== installing qt packages..."

    choco install qt5-default
else
    echo "=== jumping installation of qt packages..."
fi

# Install vcpkg if not already installed
if [ ! -d "C:/vcpkg" ]; then
    echo "=== Installing vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
    C:/vcpkg/bootstrap-vcpkg.bat
fi

# Install lcms2 via vcpkg
echo "=== Installing lcms2 via vcpkg..."
C:/vcpkg/vcpkg.exe install lcms:x64-windows
