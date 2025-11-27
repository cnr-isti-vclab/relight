#!/bin/bash
# this is a script shell sets up an ubuntu (18.04, 20.04 and 22.04) environment where
# Relight can be compiled.
#
# Run this script if you never installed any of the Relight dependencies.
# Note: Qt should be installed separately (e.g., via the Qt installer or GitHub Actions)

sudo apt-get update
echo "=== installing mesa packages..."
sudo apt-get install -y mesa-common-dev libglu1-mesa-dev 

sudo apt-get install -y cmake ninja-build patchelf fuse libjpeg-dev libeigen3-dev

# qt dependencies (for deployment)
sudo apt-get install -y libxcb-cursor0

sudo apt-get install -y liblcms2-dev
