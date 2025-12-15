#!/bin/bash
# this is a script shell sets up a Windows environment where
# RelightLab can be compiled, deployed and packaged.
#
# Run this script if you never installed any of the RelightLab dependencies.
#
# Requires: vcpkg

set -euo pipefail

echo "=== Installing dependencies via vcpkg..."

C:/vcpkg/vcpkg.exe install lcms:x64-windows 

#wget is needed for Windows redistributable
#C:/vcpkg/vcpkg.exe install wget:x64-windows

#for windows installer
#C:/vcpkg/vcpkg.exe install nsis:x64-windows

#not needed for the moment.
#C:/vcpkg/vcpkg.exe installopencv4:x64-windows
