#!/bin/bash

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"/..
INSTALL_PATH=$SCRIPTS_PATH/../../install
PACKAGES_PATH=$SCRIPTS_PATH/../../packages

#checking for parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -p=*|--packages_path=*)
        PACKAGES_PATH="${i#*=}"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSTALL_PATH/usr/lib

$SCRIPTS_PATH/resources/linuxdeploy --appdir=$INSTALL_PATH \
  --plugin qt \
  --output appimage

RELIGHT_VERSION=$(cat RELIGHT_VERSION) #get the relight version from the string

mkdir $PACKAGES_PATH
mv RelightLab-*.AppImage $PACKAGES_PATH/RelightLab$RELIGHT_VERSION-linux.AppImage
