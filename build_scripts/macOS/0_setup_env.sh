#!/bin/bash
# this is a script shell sets up a MacOS environment where
# ReLight can be compiled.
#
# Run this script if you never installed any of the ReLight dependencies.
#
# Requires: homebrew

#cmake, ninja, libtiff are already installed

brew install coreutils libomp eigen libjpeg lcms2

npm install -g appdmg

