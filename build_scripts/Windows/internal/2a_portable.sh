#!/bin/bash

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"/..
INSTALL_PATH=$SCRIPTS_PATH/../../install
QT_DIR=""
LCMS_DLL_OVERRIDE=""
LCMS_DLL_NAME="lcms2-2.dll"

#checking for parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -qt=*|--qt_dir=*)
        QT_DIR=${i#*=}/bin/
        shift # past argument=value
        ;;
    -lcms=*|--lcms_dll=*)
        LCMS_DLL_OVERRIDE="${i#*=}"
        shift
        ;;
    *)
        # unknown option
        ;;
esac
done

${QT_DIR}windeployqt $INSTALL_PATH/relightlab.exe
${QT_DIR}windeployqt $INSTALL_PATH/relight-cli.exe
${QT_DIR}windeployqt $INSTALL_PATH/relight-normals.exe
${QT_DIR}windeployqt $INSTALL_PATH/relight-merge.exe
${QT_DIR}windeployqt $INSTALL_PATH/relight-deepzoom.exe

cp C:/vcpkg/installed/x64-windows/bin/lcms2-2.dll "$INSTALL_PATH"


# Copy OpenCV DLLs, not needed for the moment.
#OPENCV_PATH="C:/tools/opencv/build/x64/vc16/bin"  # Chocolatey TODO check for vkpg
#echo "Copying OpenCV DLLs from $OPENCV_PATH to $INSTALL_PATH..."
#cp "$OPENCV_PATH"/*.dll "$INSTALL_PATH" || { echo "Failed to copy OpenCV DLLs"; exit 1; }

# remove all .lib files
for file in $(find $INSTALL_PATH -name '*.lib');
do
    rm $file
done

if [ ! -f $INSTALL_PATH/vc_redist.x64.exe ]
then
    echo "Downloading vc_redist because it was missing..."
    wget https://aka.ms/vs/17/release/vc_redist.x64.exe --output-document=$INSTALL_PATH/vc_redist.x64.exe
fi
