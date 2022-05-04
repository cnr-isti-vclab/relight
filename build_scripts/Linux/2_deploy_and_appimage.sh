#!/bin/bash
# This is a script shell for deploying a relight-portable folder, and an appimage.
# Requires a properly built relight (see 1_build.sh).
#
# Without given arguments, the folder that will be deployed is relight/install, which
# should be the path where relight has been installed (default output of 1_build.sh).
#
# You can give as argument the path where you installed relight.

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
INSTALL_PATH=$SCRIPTS_PATH/../../install
QT_DIR=""

#checking for parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -qt=*|--qt_dir=*)
        QT_DIR=${i#*=}
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

# make bundle
mkdir -p $INSTALL_PATH/usr/share/applications/
mkdir -p $INSTALL_PATH/usr/share/icons/Yaru/512x512/apps/
cp $SCRIPTS_PATH/resources/relight.desktop $INSTALL_PATH/usr/share/applications/relight.desktop
cp $SCRIPTS_PATH/../relight.png $INSTALL_PATH/usr/share/icons/Yaru/512x512/apps/relight.png

if [ ! -z "$QT_DIR" ]
then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_DIR/lib
    export QMAKE=$QT_DIR/bin/qmake
fi

chmod +x $INSTALL_PATH/usr/bin/relight
chmod +x $INSTALL_PATH/usr/bin/relight-cli
chmod +x $INSTALL_PATH/usr/bin/relight-merge

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSTALL_PATH/usr/lib
$SCRIPTS_PATH/resources/linuxdeploy --appdir=$INSTALL_PATH \
  --plugin qt --output appimage

#get version
IFS=' ' #space delimiter
STR_VERSION=$($INSTALL_PATH/AppRun --version)
echo "Version message: " $STR_VERSION
read -a strarr <<< "$STR_VERSION"
RELIGHT_VERSION=${strarr[1]} #get the relight version from the string

mv ReLight-*.AppImage ReLight$RELIGHT_VERSION-linux.AppImage
