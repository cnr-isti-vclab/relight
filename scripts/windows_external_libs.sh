#!/bin/bash

#jpeg-turbo

cd ../external
mkdir build
cd build
cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
nmake
nmake install #dir should be C:\libjpeg-turbo64
echo "======="
ls -l C:\libjpeg-turbo64
echo "======="
