#!/bin/bash
# this is a script shell sets up a MacOS environment where
# ReLight can be compiled.
#
# Run this script if you never installed any of the ReLight dependencies.
#
# Requires: homebrew

brew install coreutils cmake ninja libomp eigen libjpeg libtiff dylibbundler #opencv

npm install -g appdmg

