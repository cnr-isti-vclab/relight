#!/bin/bash
# this is a script shell for deploying a meshlab-portable app.
# Requires a properly built meshlab.
#
# Without given arguments, the folder that will be deployed is meshlab/distrib.
#
# You can give as argument the DISTRIB_PATH.
#
# After running this script, $DISTRIB_PATH/meshlab.app will be a portable meshlab application.

#default paths wrt the script folder
SCRIPTS_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

SOURCE_PATH=$SCRIPTS_PATH/../..
INSTALL_PATH=$SOURCE_PATH/install
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
        QT_DIR=${i#*=}/bin/
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

${QT_DIR}macdeployqt $INSTALL_PATH/relight.app

#get version
RL_VERSION=$(cat $SCRIPTS_PATH/../../RELIGHT_VERSION)

# final step create the dmg using appdmg
# appdmg is installed with 'npm install -g appdmg'",
sed "s%INST_PATH%$INSTALL_PATH%g" $SCRIPTS_PATH/resources/dmg_latest.json > $SCRIPTS_PATH/resources/dmg_final.json
sed -i '' "s%RL_VERSION%$RL_VERSION%g" $SCRIPTS_PATH/resources/dmg_final.json
sed -i '' "s%SOURCE_PATH%$SOURCE_PATH%g" $SCRIPTS_PATH/resources/dmg_final.json

mv $INSTALL_PATH/relight.app $INSTALL_PATH/ReLight$RL_VERSION.app

echo "Running appdmg"
appdmg $SCRIPTS_PATH/resources/dmg_final.json $INSTALL_PATH/ReLight$RL_VERSION-macos.dmg

rm $SCRIPTS_PATH/resources/dmg_final.json

#at this point, distrib folder contains a DMG ReLight file
echo "distrib folder now contains a DMG file"