#!/bin/bash

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"/../../
RESOURCES_PATH=$SCRIPTS_PATH/../../resources
SOURCE_PATH=$SCRIPTS_PATH/../../src
INSTALL_PATH=$SOURCE_PATH/../install

#check parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

mkdir -p $INSTALL_PATH/usr/share/applications/
mkdir -p $INSTALL_PATH/usr/share/icons/Yaru/512x512/apps/
cp $SCRIPTS_PATH/resources/relight.desktop $INSTALL_PATH/usr/share/applications/relight.desktop
cp $SCRIPTS_PATH/../relight.png $INSTALL_PATH/usr/share/icons/Yaru/512x512/apps/relight.png

chmod +x $INSTALL_PATH/usr/bin/relight
chmod +x $INSTALL_PATH/usr/bin/relight-cli
chmod +x $INSTALL_PATH/usr/bin/relight-merge
