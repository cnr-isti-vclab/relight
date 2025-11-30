#!/bin/bash
# this is a script shell sets up a Windows environment where
# RelightLab can be compiled, deployed and packaged.
#
# Run this script if you never installed any of the RelightLab dependencies.
#
# Requires: vcpkg

# Install vcpkg if not already installed
if [ ! -d "C:/vcpkg" ]; then
    echo "=== Installing vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
    C:/vcpkg/bootstrap-vcpkg.bat
fi

echo "=== Installing dependencies via vcpkg..."

# Libraries
C:/vcpkg/vcpkg.exe install lcms:x64-windows 
#C:/vcpkg/vcpkg.exe installopencv4:x64-windows

# Tools 
C:/vcpkg/vcpkg.exe install cmake:x64-windows ninja:x64-windows ccache:x64-windows wget:x64-windows nsis:x64-windows || true

# Export environment variables for GitHub Actions (if running in Actions)
echo "CMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake" >> "$GITHUB_ENV"
echo "QT_VERSION=6.6.*" >> "$GITHUB_ENV"

