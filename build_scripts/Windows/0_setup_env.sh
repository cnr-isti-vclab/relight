#!/bin/bash
# this is a script shell sets up a Windows environment where
# RelightLab can be compiled, deployed and packaged.
#
# Run this script if you never installed any of the RelightLab dependencies.
#
# Requires: vcpkg
# Note: Qt should be installed separately (e.g., via the Qt installer or GitHub Actions)

# Install vcpkg if not already installed
if [ ! -d "C:/vcpkg" ]; then
    echo "=== Installing vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
    C:/vcpkg/bootstrap-vcpkg.bat
fi

# Install dependencies via vcpkg
echo "=== Installing dependencies via vcpkg..."
C:/vcpkg/vcpkg.exe install lcms:x64-windows
